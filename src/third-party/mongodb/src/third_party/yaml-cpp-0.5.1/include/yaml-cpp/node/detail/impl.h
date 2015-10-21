#ifndef NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/node/detail/node.h"
#include "yaml-cpp/node/detail/node_data.h"
#include <boost/type_traits.hpp>

namespace YAML
{
	namespace detail
	{
		template<typename Key, typename Enable = void>
		struct get_idx {
			static node *get(const std::vector<node *>& /* sequence */, const Key& /* key */, shared_memory_holder /* pMemory */) {
				return 0;
			}
		};

		template<typename Key>
		struct get_idx<Key, typename boost::enable_if_c<boost::is_unsigned<Key>::value && !boost::is_same<Key, bool>::value>::type> {
			static node *get(const std::vector<node *>& sequence, const Key& key, shared_memory_holder /* pMemory */) {
				return key < sequence.size() ? sequence[key] : 0;
			}

			static node *get(std::vector<node *>& sequence, const Key& key, shared_memory_holder pMemory) {
				if(key > sequence.size())
					return 0;
				if(key == sequence.size())
					sequence.push_back(&pMemory->create_node());
				return sequence[key];
			}
		};
		
		template<typename Key>
		struct get_idx<Key, typename boost::enable_if<boost::is_signed<Key> >::type> {
			static node *get(const std::vector<node *>& sequence, const Key& key, shared_memory_holder pMemory) {
				return key >= 0 ? get_idx<std::size_t>::get(sequence, static_cast<std::size_t>(key), pMemory) : 0;
			}
			static node *get(std::vector<node *>& sequence, const Key& key, shared_memory_holder pMemory) {
				return key >= 0 ? get_idx<std::size_t>::get(sequence, static_cast<std::size_t>(key), pMemory) : 0;
			}
		};

		// indexing
		template<typename Key>
		inline node& node_data::get(const Key& key, shared_memory_holder pMemory) const
		{
			switch(m_type) {
				case NodeType::Map:
					break;
				case NodeType::Undefined:
				case NodeType::Null:
					return pMemory->create_node();
				case NodeType::Sequence:
					if(node *pNode = get_idx<Key>::get(m_sequence, key, pMemory))
						return *pNode;
					return pMemory->create_node();
				case NodeType::Scalar:
                    throw BadSubscript();
			}

			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(*it->first, key, pMemory))
					return *it->second;
			}
			
			return pMemory->create_node();
		}
		
		template<typename Key>
		inline node& node_data::get(const Key& key, shared_memory_holder pMemory)
		{
			switch(m_type) {
				case NodeType::Map:
					break;
				case NodeType::Undefined:
				case NodeType::Null:
				case NodeType::Sequence:
					if(node *pNode = get_idx<Key>::get(m_sequence, key, pMemory)) {
						m_type = NodeType::Sequence;
						return *pNode;
					}
					
					convert_to_map(pMemory);
					break;
				case NodeType::Scalar:
                    throw BadSubscript();
			}
			
			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(*it->first, key, pMemory))
					return *it->second;
			}
			
			node& k = convert_to_node(key, pMemory);
			node& v = pMemory->create_node();
			insert_map_pair(k, v);
			return v;
		}
		
		template<typename Key>
		inline bool node_data::remove(const Key& key, shared_memory_holder pMemory)
		{
			if(m_type != NodeType::Map)
				return false;
			
			for(node_map::iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(*it->first, key, pMemory)) {
					m_map.erase(it);
					return true;
				}
			}
			
			return false;
		}
        
        // map
        template<typename Key, typename Value>
        inline void node_data::force_insert(const Key& key, const Value& value, shared_memory_holder pMemory)
        {
			switch(m_type) {
				case NodeType::Map:
					break;
				case NodeType::Undefined:
				case NodeType::Null:
				case NodeType::Sequence:
					convert_to_map(pMemory);
					break;
				case NodeType::Scalar:
                    throw BadInsert();
			}
			
			node& k = convert_to_node(key, pMemory);
			node& v = convert_to_node(value, pMemory);
			insert_map_pair(k, v);
        }

		template<typename T>
		inline bool node_data::equals(node& node, const T& rhs, shared_memory_holder pMemory)
		{
			T lhs;
			if(convert<T>::decode(Node(node, pMemory), lhs))
				return lhs == rhs;
			return false;
		}
		
		inline bool node_data::equals(node& node, const char *rhs, shared_memory_holder pMemory)
		{
            return equals<std::string>(node, rhs, pMemory);
		}

        template<typename T>
		inline node& node_data::convert_to_node(const T& rhs, shared_memory_holder pMemory)
		{
			Node value = convert<T>::encode(rhs);
			value.EnsureNodeExists();
			pMemory->merge(*value.m_pMemory);
			return *value.m_pNode;
		}
	}
}

#endif // NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
