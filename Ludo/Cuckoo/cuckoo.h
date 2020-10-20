/* Copyright 2016 The TensorFlow Authors. All Rights Reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ==============================================================================*/

#pragma once

#include "../hash.h"
#include "../common.h"
#include "../Base.h"
#include "../../common.h"

// Class for efficiently storing key->value mappings when the size is   
// known in advance and the keys are pre-hashed into uint64s.
// Keys should have "good enough" randomness (be spread across the
// entire 64 bit space).
//
// Important:  Clients wishing to use deterministic keys must
// ensure that their keys fall in the range 0 .. (uint64max-1);
// the table uses 2^64-1 as the "not occupied" flag.
//
// Inserted keys must be unique, and there are no update
// or delete functions (until some subsequent use of this table
// requires them).
//
// Threads must synchronize their access to a PresizedCuckooMap.
//
// The cuckoo hash table is 4-way associative (each "bucket" has 4
// "slots" for key/value entries).  Uses breadth-first-search to find
// a good cuckoo path with less data movement (see
// http://www.cs.cmu.edu/~dga/papers/cuckoo-eurosys14.pdf )

struct CuckooMove {
  uint32_t bsrc, bdst;
  uint8_t ssrc, sdst;
};

template<class Key, class Value,int RatDown=100,int RatScan=100, int kCandidateBuckets = 2, int kSlotsPerBucket = 4>
class CuckooMap :public Base<Key,Value>{
public:

  double rtd, rts;

  bool scanned;
  
  // Utility function to compute (x * y) >> 64, or "multiply high".
  // On x86-64, this is a single instruction, but not all platforms
  // support the __uint128_t type, so we provide a generic
  // implementation as well.
  inline uint32_t multiply_high_u32(uint32_t x, uint32_t y) const {
    return (uint32_t) (((uint64_t) x * (uint64_t) y) >> 32);
  }
  
  inline uint32_t fast_map_to_buckets(uint32_t x) const {
    // Map x (uniform in 2^64) to the range [0, num_buckets_ -1]
    // using Lemire's alternative to modulo reduction:
    // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    // Instead of x % N, use (x * N) >> 64.
    return multiply_high_u32(x, num_buckets_);
  }
  
  // The key type is fixed as a pre-hashed key for this specialized use.
  explicit CuckooMap(uint32_t num_entries = 64) {
    rtd = RatDown / 100.;
    rts = RatScan / 100.;
    scanned = false;
    Clear(num_entries);
    for (auto &hash : h) {
      hash.setSeed(rand());
    }
  }
  
  inline int EntryCount() const {
    return entryCount;
  }
  
  void Clear(uint32_t num_entries) {
    entryCount = 0;
    cpq_.reset();
    num_entries /= kLoadFactor;
    num_buckets_ = (num_entries + kSlotsPerBucket - 1) / kSlotsPerBucket;
    // Very small cuckoo tables don't work, because the probability
    // of having same-bucket hashes is large.  We compromise for those
    // uses by having a larger static starting size.
    num_buckets_ += 32;
    num_slot_ = (uint32_t) num_buckets_ * kSlotsPerBucket * rts;
    num_down_ = (uint32_t) num_buckets_ * kSlotsPerBucket * rtd;
    Bucket empty_bucket;
    buckets_.clear();
    buckets_.resize(num_buckets_, empty_bucket);
  }
  
  pair<int, int> locate(const Key &k) const {
    for (int i = 0; i < kCandidateBuckets; ++i) {
      uint32_t bucket = fast_map_to_buckets(h[i](k));
      
      const Bucket &bref = buckets_[bucket];
      for (int slot = 0; slot < kSlotsPerBucket; slot++) {
        if ((bref.occupiedMask & (1U << slot)) && (bref.keys[slot] == k)) {
          return make_pair((int) bucket, slot);
        }
      }
    }
    
    return make_pair(-1, -1);
  }
  
  // Returns collided key if some key collides with the key being inserted;
  // returns null if the table is full; returns &k if inserted successfully.
    int insert(const Key &k, const Value &v) {
    // Merged find and duplicate checking.
//    printf("current insert %d %d\n",k,v);
//    printf("%d\n",buckets_[170].occupiedMask);
    uint32_t target_bucket;
    int target_slot = -1;     
    entryCount++;
    
    for (int i = 0; i < kCandidateBuckets ; ++i) {
      uint32_t bucket = fast_map_to_buckets(h[i](k));
      Bucket *bptr = &buckets_[bucket];
      for (int slot = 0; slot < kSlotsPerBucket; slot++) {
        if ((bptr->occupiedMask & (1ULL << slot))) {
          if (bptr->keys[slot]==k)
          {
            if (bptr->values[slot]==-1)
            {
              target_bucket = bucket;
              target_slot = slot;             
            }
            else
            {
              Counter::count("Cuckoo collision");
//              printf("collision!");
              return EXISTS;
            }
          }
        } else if (target_slot == -1) {
          target_bucket = bucket;
          target_slot = slot; 
        }
      }
    }
    
    if (target_slot != -1) {
      Counter::count("Cuckoo direct insert");
//      printf("%d %d\n",target_bucket,target_slot);
      InsertInternal(k, v, target_bucket, target_slot);
//      printf("%d\n",buckets_[target_bucket].keys[target_slot]);
//    printf("%d %d\n",target_bucket,buckets_[target_bucket].occupiedMask);
      return OK;
    }
    
    // No space, perform cuckooInsert
    Counter::count("Cuckoo cuckoo insert");
    
    if (CuckooInsert(k, v)) return OK;
     else {
      Counter::count("Cuckoo insert fail");
      entryCount--;
      return FULL;
    }
  }
  
  inline int erase(const Key &k) {
//	printf("current deletion %d\n",k);
    for (int i = 0; i < kCandidateBuckets; ++i) {
      uint32_t bucket = fast_map_to_buckets(h[i](k));
      if (RemoveInBucket(k, bucket)) return OK;
    }
    Counter::count("Cuckoo uncertain deletion");
    return insert(k, -1);
  }
  
  // Returns true if found.  Sets *out = value.
  inline int lookup(const Key &k,Value &out) {
//	 printf("current lookup %d\n",k);
//      printf("%d\n",buckets_[170].occupiedMask);
    for (int i = 0; i < kCandidateBuckets; ++i) {
      uint32_t bucket = fast_map_to_buckets(h[i](k));
      if (FindInBucket(k, bucket, out)) return true;
    }
    
    return false;
  }
  
  inline int modify(const Key &k, const Value& v) {
//	 printf("current modify %d %d\n",k,v);
//   printf("%d\n",buckets_[170].occupiedMask);
    for (int i = 0; i < kCandidateBuckets; ++i) {
      uint32_t b = fast_map_to_buckets(h[i](k));
      Bucket &bref = buckets_[b];
//      printf("%d\n",bref.occupiedMask);
      for (int i = 0; i < kSlotsPerBucket; i++) {
        if ((bref.occupiedMask & (1U << i)) && (bref.keys[i] == k)) {
          bref.values[i] = v;
          return OK;
        }
      }
    }
    Counter::count("Cuckoo uncertain updating");
    return insert(k, v);
  }

  int Merge(unordered_map<Key,Value,Hasher32<Key> >& other)
  {
        return ERROR;
  }
  
  unordered_map<Key, Value, Hasher32<Key> > toMap() {
    unordered_map<Key, Value, Hasher32<Key> > map;
    scanned = false;
    uint32_t ElementCounter=0;
    for (auto &bucket: buckets_) {  // all buckets
      if (ElementCounter>=num_down_) break;
      for (int slot = 0; slot < kSlotsPerBucket; ++slot) 
        if ((bucket.occupiedMask & (1ULL << slot)) && !(bucket.clockalgMask & (1ULL << slot)))
        {
          map.insert(make_pair(bucket.keys[slot], bucket.values[slot]));
          bucket.occupiedMask ^= (1ULL << slot);
          ++ElementCounter;
          if (ElementCounter>=num_down_) break;
        }
    }
    for (auto &bucket: buckets_) {  // all buckets
      if (ElementCounter>=num_down_) break;
      for (int slot = 0; slot < kSlotsPerBucket; ++slot) 
        if ((bucket.occupiedMask & (1ULL << slot)))
        {
          map.insert(make_pair(bucket.keys[slot], bucket.values[slot]));
          bucket.occupiedMask ^= (1ULL << slot);
          bucket.clockalgMask ^= (1ULL << slot);
          ++ElementCounter;
          if (ElementCounter>=num_down_) break;
        }
    }
    
    return map;
  }
  
  uint64_t getMemoryCost() const {
    return num_buckets_ * sizeof(buckets_[0]);
  }
  
  Hasher32<Key> h[kCandidateBuckets + 1];   // the last h is the digest function used in associated data plane
  
  // The load factor is chosen slightly conservatively for speed and
  // to avoid the need for a table rebuild on insertion failure.
  // 0.94 is achievable, but 0.85 is faster and keeps the code simple
  // at the cost of a small amount of memory.
  // NOTE:  0 < kLoadFactor <= 1.0
  static constexpr double kLoadFactor = 0.95;
  
  // Cuckoo insert:  The maximum number of entries to scan should be ~400
  // (Source:  Personal communication with Michael Mitzenmacher;  empirical
  // experiments validate.).  After trying 400 candidate locations, declare
  // the table full - it's probably full of unresolvable cycles.  Less than
  // 400 reduces max occupancy;  much more results in very poor performance
  // around the full point.  For (2,4) a max BFS path len of 5 results in ~682
  // nodes to visit, calculated below, and is a good value.
  
  static constexpr uint8_t kMaxBFSPathLen = 5;
  
  // Constants for BFS cuckoo path search:
  // The visited list must be maintained for all but the last level of search
  // in order to trace back the path. The BFS search has two roots
  // and each can go to a total depth (including the root) of 5.
  // The queue must be sized for 4 * \sum_{k=0...4}{(3*kSlotsPerBucket)^k}.
  // The visited queue, however, does not need to hold the deepest level,
  // and so it is sized 4 * \sum{k=0...3}{(3*kSlotsPerBucket)^k}
  static constexpr int calMaxQueueSize() {
    int result = 0;
    int term = 4;
    for (int i = 0; i < kMaxBFSPathLen; ++i) {
      result += term;
      term *= ((kCandidateBuckets - 1) * kSlotsPerBucket);
    }
    return result;
  }
  
  static constexpr int calVisitedListSize() {
    int result = 0;
    int term = 4;
    for (int i = 0; i < kMaxBFSPathLen - 1; ++i) {
      result += term;
      term *= ((kCandidateBuckets - 1) * kSlotsPerBucket);
    }
    return result;
  }
  
  static constexpr int kMaxQueueSize = calMaxQueueSize();
  static constexpr int kVisitedListSize = calVisitedListSize();
  
  struct Bucket {
  public:
    uint8_t occupiedMask = 0;
    uint8_t clockalgMask = 0;
    Key keys[kSlotsPerBucket];
    Value values[kSlotsPerBucket];
  };
  
  int entryCount = 0;
  // Insert uses the BFS optimization (search before moving) to reduce
  // the number of cache lines dirtied during search.
  
  struct CuckooPathEntry {
    uint32_t bucket;
    int depth;
    int parent;      // To index in the visited array.
    int parent_slot; // Which slot in our parent did we come from?  -1 == root.
  };
  
  // CuckooPathQueue is a trivial circular queue for path entries.
  // The caller is responsible for not inserting more than kMaxQueueSize
  // entries.  Each PresizedCuckooMap has one (heap-allocated) CuckooPathQueue
  // that it reuses across inserts.
  class CuckooPathQueue {
  public:
    CuckooPathQueue()
      : head_(0), tail_(0) {
    }
    
    void push_back(CuckooPathEntry e) {
      queue_[tail_] = e;
      tail_ = (tail_ + 1) % kMaxQueueSize;
    }
    
    CuckooPathEntry pop_front() {
      CuckooPathEntry &e = queue_[head_];
      head_ = (head_ + 1) % kMaxQueueSize;
      return e;
    }
    
    bool empty() const {
      return head_ == tail_;
    }
    
    bool full() const {
      return ((tail_ + 1) % kMaxQueueSize) == head_;
    }
    
    void reset() {
      head_ = tail_ = 0;
    }
  
  private:
    CuckooPathEntry queue_[kMaxQueueSize];
    int head_;
    int tail_;
  };
  
  inline void ClearAllBits()
  {
        scanned = true;
        for (auto &bucket: buckets_) bucket.clockalgMask = 0;
  }
  
  inline void InsertInternal(const Key &k, const Value &v,uint32_t b, int slot) {
    Bucket &bptr = buckets_[b];
    bptr.keys[slot] = k;
    bptr.values[slot] = v;
    bptr.occupiedMask |= 1U << slot;
    bptr.clockalgMask |= (v!=-1) << slot;
    if (!scanned && entryCount >= num_slot_) ClearAllBits();
  }
  
  // For the associative cuckoo table, check all of the slots in
  // the bucket to see if the key is present.
  inline int RemoveInBucket(const Key &k, uint32_t b) {
    Bucket &bref = buckets_[b];
    for (int i = 0; i < kSlotsPerBucket; i++) {
      if ((bref.occupiedMask & (1U << i)) && bref.keys[i] == k) {
        if (bref.values[i]==-1)
        Counter::count("Cuckoo Error element is already deleted");
        bref.values[i]=-1;
        bref.clockalgMask &= ~(1U << i);
        return true;
      }
    }
    return false;
  }
  
  // For the associative cuckoo table, check all of the slots in
  // the bucket to see if the key is present.
  inline bool FindInBucket(const Key &k, uint32_t b, Value &out) {
    Bucket &bref = buckets_[b];
    for (int i = 0; i < kSlotsPerBucket; i++) {
      if ((bref.occupiedMask & (1U << i)) && (bref.keys[i] == k)) {
        out = bref.values[i];
        bref.clockalgMask |= (1U << i);
        return true;
      }
    }
    return false;
  }
  
  //  returns either -1 or the index of an
  //  available slot (0 <= slot < kSlotsPerBucket)
  inline int FindFreeSlot(uint32_t bucket) const {
    const Bucket &bref = buckets_[bucket];
    for (int i = 0; i < kSlotsPerBucket; i++) {
      if (!(bref.occupiedMask & (1U << i))) {
        return i;
      }
    }
    return -1;
  }
  
  inline void CopyItem(uint32_t src_bucket, int src_slot, uint32_t dst_bucket, int dst_slot) {
    Counter::count("Cuckoo copy item");
    Bucket &src_ref = buckets_[src_bucket];
    Bucket &dst_ref = buckets_[dst_bucket];
    dst_ref.keys[dst_slot] = src_ref.keys[src_slot];
    dst_ref.values[dst_slot] = src_ref.values[src_slot];
    (dst_ref.clockalgMask &= ~(1U << dst_slot)) |= ((src_ref.clockalgMask >> src_slot) & 1) << dst_slot; 
  }
  
  /// @return cuckoo path if rememberPath is true. or {1} to indicate success and {} to indicate fail.
  bool CuckooInsert(const Key &k, const Value &v) {
    fprintf(stderr,"Cuckoo Insert!\n");
    int visited_end = -1;
    cpq_.reset();
    
    for (int i = 0; i < kCandidateBuckets; ++i) {
      uint32_t bucket = fast_map_to_buckets(h[i](k));
      cpq_.push_back({bucket, 1, -1, -1}); // Note depth starts at 1.
    }
    
    while (!cpq_.empty()) {
      CuckooPathEntry entry = cpq_.pop_front();
      int free_slot = FindFreeSlot(entry.bucket);
      if (free_slot != -1) {
        // found a free slot in this path. just insert and follow this path
        buckets_[entry.bucket].occupiedMask |= 1U << free_slot;
        while (entry.depth > 1) { 
          // "copy" instead of "swap" because one entry is always zero.
          // After, write target key/value over top of last copied entry.
          CuckooPathEntry parent = visited_[entry.parent];
          CopyItem(parent.bucket, entry.parent_slot, entry.bucket, free_slot);
          free_slot = entry.parent_slot;
          entry = parent;
        }
        
        InsertInternal(k, v, entry.bucket, free_slot);
        
        return true;
      } else if (entry.depth < kMaxBFSPathLen) {
        visited_[++visited_end] = entry;
        auto parent_index = visited_end;
        
        // Don't always start with the same slot, to even out the path depth.
        int start_slot = (entry.depth + entry.bucket) % kSlotsPerBucket;
        const Bucket &bref = buckets_[entry.bucket];
        
        for (int i = 0; i < kSlotsPerBucket; i++) {
          int slot = (start_slot + i) % kSlotsPerBucket;
          
          for (int j = 0; j < kCandidateBuckets; ++j) {
            uint32_t next_bucket = fast_map_to_buckets(h[j](bref.keys[slot]));
            if (next_bucket == entry.bucket) continue;
            
            cpq_.push_back({next_bucket, entry.depth + 1, parent_index, slot});
          }
        }
      }
    }
    
    return false;
  }
  
  // Set upon initialization: num_entries / kLoadFactor / kSlotsPerBucket.
  uint32_t num_buckets_, num_slot_, num_down_;
  std::vector<Bucket> buckets_;
  
  CuckooPathQueue cpq_;
  CuckooPathEntry visited_[kVisitedListSize];
};
