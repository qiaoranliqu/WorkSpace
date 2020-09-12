#include "fawnds_sf_ordered_trie.h"
#include "debug.h"
#include "print.h"
#include <sstream>

namespace fawn
{
	FawnDS_SF_Ordered_Trie::FawnDS_SF_Ordered_Trie()
		: index_(NULL), data_store_(NULL)
	{
	}

	FawnDS_SF_Ordered_Trie::~FawnDS_SF_Ordered_Trie()
	{
		if (index_)
			Close();
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Create()
	{
		if (index_)
			return ERROR;

        key_len_ = atoi(config_->GetStringValue("child::key-len").c_str());
        data_len_ = atoi(config_->GetStringValue("child::data-len").c_str());
        keys_per_block_ = atoi(config_->GetStringValue("child::keys-per-block").c_str());

        size_ = atoi(config_->GetStringValue("child::size").c_str());
        bucket_size_ = atoi(config_->GetStringValue("child::bucket-size").c_str());

        skip_bits_ = atoi(config_->GetStringValue("child::skip-bits").c_str());

		actual_size_ = 0;

		if (key_len_ == 0) {
			fprintf(stderr, "FawnDS_SF_Ordered_Trie::Create(): invalid key length\n");
			return ERROR;
		}

		if (data_len_ == 0) {
			fprintf(stderr, "FawnDS_SF_Ordered_Trie::Create(): invalid data length\n");
			return ERROR;
		}

		if (keys_per_block_ == 0) {
			fprintf(stderr, "FawnDS_SF_Ordered_Trie::Create(): invalid keys per block\n");
			return ERROR;
		}

		// the size can be zero
		/*
		if (size_ == 0) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Create(): invalid size\n");
			return ERROR;
		}
		*/

		if (bucket_size_ == 0) {
			fprintf(stderr, "FawnDS_SF_Ordered_Trie::Create(): invalid bucket size\n");
			return ERROR;
		}

		DPRINTF(2, "FawnDS_SF_Ordered_Trie::Create(): creating index\n");

		index_ = new index_type(key_len_, size_, bucket_size_, 0, keys_per_block_, skip_bits_);

		DPRINTF(2, "FawnDS_SF_Ordered_Trie::Create(): creating data store\n");

		Configuration* storeConfig = new Configuration(config_, true);
		storeConfig->SetContextNode("child::datastore");
		storeConfig->SetStringValue("child::id", config_->GetStringValue("child::id"));
        char buf[1024];
        //snprintf(buf, sizeof(buf), "%zu", key_len_ + data_len_);
        snprintf(buf, sizeof(buf), "%zu", key_len_ + data_len_ + 4);    // HACK: alignment for 1020-byte entry
        storeConfig->SetStringValue("child::data-len", buf);
		data_store_ = FawnDS_Factory::New(storeConfig);
		if (data_store_ == NULL) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Create(): could not create data store\n");
			return ERROR;
		}

		FawnDS_Return ret = data_store_->Create();
		if (ret != OK)
			return ret;

		DPRINTF(2, "FawnDS_SF_Ordered_Trie::Create(): <result> done\n");

		return OK;
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Open()
	{
		// TODO: implement
		return ERROR;
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Flush()
	{
		if (!index_)
			return ERROR;

		if (!index_->finalized())
			index_->flush();
		else {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Flush(): already finalized\n");
		}

		return OK;
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Close()
	{
		if (!index_)
			return OK;

		Flush();

		delete index_;
		index_ = NULL;
		delete data_store_;
		data_store_ = NULL;

		return OK;
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Destroy()
	{
		Configuration* storeConfig = new Configuration(config_, true);
		storeConfig->SetContextNode("child::datastore");
		storeConfig->SetStringValue("child::id", config_->GetStringValue("child::id"));
		FawnDS* data_store = FawnDS_Factory::New(storeConfig);
		FawnDS_Return ret_destroy_data_store = data_store->Destroy();
		delete data_store;

		return ret_destroy_data_store;
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Status(const FawnDS_StatusType& type, Value& status) const
	{
		if (!index_)
			return ERROR;

		std::ostringstream oss;
		switch (type) {
		case NUM_DATA:
			oss << actual_size_;
			break;
		case NUM_ACTIVE_DATA:
			oss << actual_size_;
			break;
		case CAPACITY:
			oss << size_;
			break;
		case MEMORY_USE:
			oss << '[' << (index_->bit_size() + 7) / 8 << ",0]";
			break;
		case DISK_USE:
			{
				Value status_part;
				if (data_store_->Status(type, status_part) != OK)
					return UNSUPPORTED;
				oss << "[0," << status_part.str() << ']';
			}
			break;
		default:
			return UNSUPPORTED;
		}
		status = NewValue(oss.str());
		return OK;
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Put(const ConstValue& key, const ConstValue& data)
	{
#ifdef DEBUG
    if (debug_level & 2) {
        DPRINTF(2, "FawnDS_SF_Ordered_Trie::Put(): key=\n");
        print_payload((const u_char*)key.data(), key.size(), 4);
        DPRINTF(2, "FawnDS_SF_Ordered_Trie::Put(): data=\n");
        print_payload((const u_char*)data.data(), data.size(), 4);
    }
#endif

		if (!index_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Put(): <result> not initialized\n");
			return ERROR;
		}

		if (index_->finalized()) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Put(): <result> already finalized\n");
			return ERROR;
		}

		if (key.size() != key_len_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Put(): <result> key length mismatch\n");
			return INVALID_KEY;
		}
		if (data.size() != data_len_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Put(): <result> data length mismatch\n");
			return INVALID_DATA;
		}

		if (!index_->insert(reinterpret_cast<const uint8_t*>(key.data()))) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Put(): <result> cannot insert key to index, probably non-sorted key\n");
			return INVALID_KEY;
		}

		Value combined_kv;
		//combined_kv.resize(key_len_ + data_len_);
		combined_kv.resize(key_len_ + data_len_ + 4);   // HACK: alignment for 1020-byte entry
		memcpy(combined_kv.data(), key.data(), key_len_);
		memcpy(combined_kv.data() + key_len_, data.data(), data_len_);
        memset(combined_kv.data() + key_len_ + data_len_, 0, 4);   // HACK: alignment for 1020-byte entry

		SizedValue<64> data_store_key;
		data_store_->Append(data_store_key, combined_kv);

		actual_size_++;

		DPRINTF(2, "FawnDS_SF_Ordered_Trie::Put(): <result> put new key\n");
		return OK;
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Contains(const ConstValue& key) const
	{
		if (!index_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Contains(): <result> not initialized\n");
			return ERROR;
		}

		if (!index_->finalized()) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Contains(): <result> not finalized\n");
			return ERROR;
		}

		if (key.size() != key_len_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Contains(): <result> key length mismatch\n");
			return INVALID_KEY;
		}

		// TODO: better implementation
		SizedValue<64> data;
		FawnDS_Return ret_get = Get(key, data);
		return ret_get;
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Length(const ConstValue& key, size_t& len) const
	{
		if (!index_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Length(): <result> not initialized\n");
			return ERROR;
		}

		if (!index_->finalized()) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Length(): <result> not finalized\n");
			return ERROR;
		}

		if (key.size() != key_len_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Length(): <result> key length mismatch\n");
			return INVALID_KEY;
		}

		// TODO: better implementation
		len = data_len_;
		return Contains(key);
	}

	FawnDS_Return
	FawnDS_SF_Ordered_Trie::Get(const ConstValue& key, Value& data, size_t offset, size_t len) const
	{
		if (!index_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): <result> not initialized\n");
			return ERROR;
		}

		if (!index_->finalized()) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): <result> not finalized\n");
			return ERROR;
		}

		if (key.size() == 0)
			return INVALID_KEY;

		if (key.size() != key_len_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): <result> key length mismatch\n");
			return INVALID_KEY;
		}

#ifdef DEBUG
		if (debug_level & 2) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): key=\n");
			print_payload((const u_char*)key.data(), key.size(), 4);
		}
#endif

		size_t base_idx = index_->locate(reinterpret_cast<const uint8_t*>(key.data()));
		base_idx = base_idx / keys_per_block_ * keys_per_block_;

		for (size_t i = base_idx; i < base_idx + keys_per_block_; i++) {

			size_t data_store_key = i;
			SizedValue<64> ret_key;

			FawnDS_Return ret_get = data_store_->Get(RefValue(&data_store_key), ret_key, 0, key_len_);

			if (ret_get == END) {
				DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): got END, corrupted file store?\n");
				break;
			}

			if (ret_get != OK) {
				DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): got non-OK, corrupted file store?\n");
                //fprintf(stderr, "%zu error %zu\n", base_idx, ret_get);
				return ret_get;
			}
			if (ret_key.size() != key_len_) {
				DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): got wrong key size, corrupted file store?\n");
				return ERROR;
			}
			
			if (key != ret_key) {
				DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): key mismatch; continue\n");
				continue;
			}

			if (offset > data_len_)
				return END;

			if (offset + len > data_len_)
				len = data_len_ - offset;

			ret_get = data_store_->Get(RefValue(&data_store_key), data, key_len_, data_len_);

			if (ret_get != OK) {
                //fprintf(stderr, "%zu error %zu\n", base_idx, ret_get);
				DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): got non-OK, corrupted file store?\n");
				return ret_get;
			}
			if (data.size() != data_len_) {
				DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): got wrong data size, corrupted file store?\n");
				return ERROR;
			}
#ifdef DEBUG
			if (debug_level & 2) {
				DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): <result> data=\n");
				print_payload((const u_char*)data.data(), data.size(), 4);
			}
#endif
            //fprintf(stderr, "%zu found\n", base_idx);
			return OK;
		}

        //fprintf(stderr, "%zu not found\n", base_idx);
		DPRINTF(2, "FawnDS_SF_Ordered_Trie::Get(): <result> cannot find key\n");
		return KEY_NOT_FOUND;
	}

	FawnDS_ConstIterator
	FawnDS_SF_Ordered_Trie::Enumerate() const
	{
		if (!index_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Enumerate(): <result> not initialized\n");
			return FawnDS_ConstIterator();
		}

		if (!index_->finalized()) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Enumerate(): <result> not finalized\n");
			return FawnDS_ConstIterator();
		}

		IteratorElem* elem = new IteratorElem(this);
		elem->data_store_it = data_store_->Enumerate();
		elem->Parse();
		return FawnDS_ConstIterator(elem);
	}

	FawnDS_Iterator
	FawnDS_SF_Ordered_Trie::Enumerate()
	{
		if (!index_) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Enumerate(): <result> not initialized\n");
			return FawnDS_Iterator();
		}

		if (!index_->finalized()) {
			DPRINTF(2, "FawnDS_SF_Ordered_Trie::Enumerate(): <result> not finalized\n");
			return FawnDS_Iterator();
		}

		IteratorElem* elem = new IteratorElem(this);
		elem->data_store_it = data_store_->Enumerate();
		elem->Parse();
		return FawnDS_Iterator(elem);
	}

	FawnDS_SF_Ordered_Trie::IteratorElem::IteratorElem(const FawnDS_SF_Ordered_Trie* fawnds)
	{
		this->fawnds = fawnds;
	}

	FawnDS_IteratorElem*
	FawnDS_SF_Ordered_Trie::IteratorElem::Clone() const
	{
		IteratorElem* elem = new IteratorElem(static_cast<const FawnDS_SF_Ordered_Trie*>(fawnds));
		*elem = *this;
		return elem;
	}

	void
	FawnDS_SF_Ordered_Trie::IteratorElem::Next()
	{
		++data_store_it;

		Parse();
	}

	void
	FawnDS_SF_Ordered_Trie::IteratorElem::Parse()
	{
		if (data_store_it.IsOK()) {
			state = OK;
			const FawnDS_SF_Ordered_Trie* fawnds = static_cast<const FawnDS_SF_Ordered_Trie*>(this->fawnds);

			Value& data_store_data = data_store_it->data;

			key = NewValue(data_store_data.data(), fawnds->key_len_);
			data = NewValue(data_store_data.data() + fawnds->key_len_, fawnds->data_len_);
		}
		else if (data_store_it.IsEnd())
			state = END;
		else
			state = ERROR;
	}

	/*
    template <typename BucketingType>
    void
    FawnDS_SF_Ordered_Trie<BucketingType>::LoadFromFile()
    {
        if (initialized_) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: LoadFromFile: already initialized\n");
            return;
        }

        string filename = config_->getStringValue("file");
        int fd;
        if ((fd = open(filename.c_str(), O_RDONLY | O_NOATIME, 0666)) == -1) {
            perror("FawnDS_SF_Ordered_Trie: LoadFromFile: could not open index file");
            return;
        }
        
        // read base offset
        if (read(fd, &base_data_store_offset_, sizeof(base_data_store_offset_)) != sizeof(base_data_store_offset_)) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: LoadFromFile: unable to read base offset\n");
            close(fd);
            return;
        }

        // read hash table size
        if (read(fd, &hash_table_size_, sizeof(hash_table_size_)) != sizeof(hash_table_size_)) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: LoadFromFile: unable to read hash table size\n");
            close(fd);
            return;
        }

        // read actual hash table size (inferrable, but for compatiblity with binary search)
        if (read(fd, &actual_size_, sizeof(actual_size_)) != sizeof(actual_size_)) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: LoadFromFile: unable to read actual hash table size\n");
            close(fd);
            return;
        }

        group_bits_ = 0;
        while ((1u << (++group_bits_)) * group_size_ < hash_table_size_);

        // read hash function offsets
        for (size_t i = 0; i < (1u << group_bits_) + 1; i++) {
            hash_func_index_type offset;
            if (read(fd, &offset, sizeof(offset)) != sizeof(offset)) {
                fprintf(stderr, "FawnDS_SF_Ordered_Trie: LoadFromFile: unable to read offset\n");
                close(fd);
                return;
            }
            hash_func_offsets_.push_back(offset);
        }

        // read datastore offsets
        for (size_t i = 0; i < (1u << group_bits_) + 1; i++) {
            data_store_index_type offset;
            if (read(fd, &offset, sizeof(offset)) != sizeof(offset)) {
                fprintf(stderr, "FawnDS_SF_Ordered_Trie: LoadFromFile: unable to read offset\n");
                close(fd);
                return;
            }
            data_store_offsets_.push_back(offset);
        }

        // read hash function
        hash_func_index_type end = hash_func_offsets_[(1u << group_bits_)];
        uint8_t b;
        for (size_t i = 0; i < end; i += sizeof(b) * 8) {
            if (read(fd, &b, sizeof(b)) != sizeof(b)) {
                fprintf(stderr, "FawnDS_SF_Ordered_Trie: LoadFromFile: unable to read hash function byte\n");
                close(fd);
                return;
            }
            for (size_t j = 0; j < sizeof(b) * 8 && i + j < end; j++)
                hash_funcs_.push_back(trie::bit_access<uint8_t>::get(&b, j));
        }

        close(fd);
        finalized_ = true;

        initialized_ = true;
        fprintf(stdout, "FawnDS_SF_Ordered_Trie: LoadFromFile: loaded\n");

#ifdef DEBUG
        fprintf(stdout, "FawnDS_SF_Ordered_Trie: LoadFromFile: state: %lu\n", hash_state());
#endif
    }

    template <typename BucketingType>
    void
    FawnDS_SF_Ordered_Trie<BucketingType>::StoreToFile()
    {
        if (!initialized_) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: StoreToFile: not initailized\n");
            return;
        }

        if (!finalized_) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: StoreToFile: not finalized\n");
            return;
        }

        string filename = config_->getStringValue("file");
        int fd;
        if ((fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOATIME, 0666)) == -1) {
            perror("FawnDS_SF_Ordered_Trie: StoreToFile: could not open index file");
            return;
        }

        // write base offset
        if (write(fd, &base_data_store_offset_, sizeof(base_data_store_offset_)) != sizeof(base_data_store_offset_)) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: StoreToFile: unable to write base offset\n");
            close(fd);
            return;
        }

        // write hash table size
        if (write(fd, &hash_table_size_, sizeof(hash_table_size_)) != sizeof(hash_table_size_)) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: StoreToFile: unable to write hash table size\n");
            close(fd);
            return;
        }

        // write actual hash table size
        if (write(fd, &actual_size_, sizeof(actual_size_)) != sizeof(actual_size_)) {
            fprintf(stderr, "FawnDS_SF_Ordered_Trie: StoreToFile: unable to write actual hash table size\n");
            close(fd);
            return;
        }

        // write hash function offsets
        assert(hash_func_offsets_.size() == (1u << group_bits_) + 1);
        for (size_t i = 0; i < (1u << group_bits_) + 1; i++) {
            hash_func_index_type offset = hash_func_offsets_[i];
            if (write(fd, &offset, sizeof(offset)) != sizeof(offset)) {
                fprintf(stderr, "FawnDS_SF_Ordered_Trie: StoreToFile: unable to write offset\n");
                close(fd);
                return;
            }
        }

        // write datastore offsets
        assert(data_store_offsets_.size() == (1u << group_bits_) + 1);
        for (size_t i = 0; i < (1u << group_bits_) + 1; i++) {
            data_store_index_type offset = data_store_offsets_[i];
            if (write(fd, &offset, sizeof(offset)) != sizeof(offset)) {
                fprintf(stderr, "FawnDS_SF_Ordered_Trie: StoreToFile: unable to write offset\n");
                close(fd);
                return;
            }
        }

        // write hash function
        hash_func_index_type end = hash_func_offsets_[(1u << group_bits_)];
        assert(hash_funcs_.size() == end);
        uint8_t b;
        for (size_t i = 0; i < end; i += sizeof(b) * 8) {
            b = 0;
            for (size_t j = 0; j < sizeof(b) * 8 && i + j < end; j++)
            {
                if (hash_funcs_[i + j])
                    trie::bit_access<uint8_t>::set(&b, j);
                else
                    trie::bit_access<uint8_t>::reset(&b, j);
            }
            if (write(fd, &b, sizeof(b)) != sizeof(b)) {
                fprintf(stderr, "FawnDS_SF_Ordered_Trie: StoreToFile: unable to write hash function byte\n");
                close(fd);
                return;
            }
        }

        close(fd);
        fprintf(stdout, "FawnDS_SF_Ordered_Trie: StoreToFile: stored\n");

#ifdef DEBUG
        fprintf(stdout, "FawnDS_SF_Ordered_Trie: StoreToFile: state: %lu\n", hash_state());
#endif
    }
	*/

}  // namespace fawn
