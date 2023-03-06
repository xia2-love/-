#ifndef RADIX_TREE_HPP
#define RADIX_TREE_HPP

#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "radix_tree_it.hpp"
#include "radix_tree_node.hpp"
#include <functional>

template<typename K>
K radix_substr(const K& key, int begin, int num);

template<>
inline std::string radix_substr<std::string>(const std::string& key, int begin, int num)
{
    return key.substr(begin, num);
}

template<typename K>
K radix_join(const K& key1, const K& key2);

template<>
inline std::string radix_join<std::string>(const std::string& key1, const std::string& key2)
{
    return key1 + key2;
}

template<typename K>
int radix_length(const K& key);

template<>
inline int radix_length<std::string>(const std::string& key)
{//��size_tת��Ϊint
    return static_cast<int>(key.size());
}

template <typename K, typename T, typename Compare>
class radix_tree {
public:
    typedef K key_type;
    typedef T mapped_type;
    typedef std::pair<const K, T> value_type;
    typedef radix_tree_it<K, T, Compare>   iterator;
    typedef std::size_t  size_type;
    //���캯�����ò����б��ʼ����Ա �޲�������
    radix_tree() : m_size(0), m_root(NULL), m_predicate(Compare()) { }
    //��ֹ��ʽת�� �в�������
    explicit radix_tree(Compare pred) : m_size(0), m_root(NULL), m_predicate(pred) { }
    //��������
    ~radix_tree() {
        delete m_root;
    }
    //m_size��ʾ���������м����ַ���
    size_type size()  const {
        return m_size;
    }
    bool empty() const {
        return m_size == 0;
    }
    void clear() {
        delete m_root;
        m_root = NULL;
        m_size = 0;
    }

    iterator find(const K& key);
    iterator begin();
    iterator end();
    iterator longest_match(const K& key);

    std::pair<iterator, bool> insert(const value_type& val);
    bool erase(const K& key);
    void erase(iterator it);
    void prefix_match(const K& key, std::vector<iterator>& vec);
    void greedy_match(const K& key, std::vector<iterator>& vec);
    

    T& operator[] (const K& lhs);

    template<class _UnaryPred> void remove_if(_UnaryPred pred)
    {
        radix_tree<K, T, Compare>::iterator backIt;
        for (radix_tree<K, T, Compare>::iterator it = begin(); it != end(); it = backIt)
        {
            backIt = it;
            backIt++;
            //����ǰ������ָ��ļ�ֵ��ֵ��toDelete
            K toDelete = (*it).first;
            if (pred(toDelete))
            {
                erase(toDelete);
            }
        }
    }


private:
    size_type m_size;
    radix_tree_node<K, T, Compare>* m_root;
    Compare m_predicate;

    radix_tree_node<K, T, Compare>* begin(radix_tree_node<K, T, Compare>* node);
    radix_tree_node<K, T, Compare>* find_node(const K& key, radix_tree_node<K, T, Compare>* node, int depth);
    //����ֵ׷���ڸ�ĸ�ڵ�ĺ��棬������Ҷ�ӽڵ�
    radix_tree_node<K, T, Compare>* append(radix_tree_node<K, T, Compare>* parent, const value_type& val);
    //�����ԭ���ڵ㲻ͬ�Ľڵ㣬�������ڵ㣬��󷵻�Ҷ�ӽڵ�
    radix_tree_node<K, T, Compare>* prepend(radix_tree_node<K, T, Compare>* node, const value_type& val);
    void greedy_match(radix_tree_node<K, T, Compare>* node, std::vector<iterator>& vec);
    //���ƺ����͸�ֵ����
    radix_tree(const radix_tree& other); // delete
    radix_tree& operator =(const radix_tree other); // delete
};

template <typename K, typename T, typename Compare>
void radix_tree<K, T, Compare>::prefix_match(const K& key, std::vector<iterator>& vec)
{//ǰ׺ƥ���˼�����ҵ������뵱ǰ��ֵ��ͬ����ַ��������ҳ���
    vec.clear();
    //���ڵ�Ϊ�գ�����ǰ׺ƥ��
    if (m_root == NULL)
        return;

    radix_tree_node<K, T, Compare>* node;
    K key_sub1, key_sub2;
    //��m_root���ڵ㣬���Ϊ0��ʼ�Ҽ�ֵkey�����ؽڵ�ָ��node
    node = find_node(key, m_root, 0);
    //����ýڵ���Ҷ�ӽڵ㣬�ڵ�ָ���丸ĸ�ڵ�
    if (node->m_is_leaf)
        node = node->m_parent;
    //��ֵ�ĳ���-�ڵ�����
    int len = radix_length(key) - node->m_depth;
    //key_sub1��ʾ��ֵ���಻��ͬ�Ĳ��֣�key_sub2��ʾnode��node->m_key��0��ʼ��len���ȵĲ���
    key_sub1 = radix_substr(key, node->m_depth, len);
    key_sub2 = radix_substr(node->m_key, 0, len);
    //������߲���ͬ������ǰ׺�����أ�����ָ��̰��ƥ�䣬���Ƿ��и����ǰ׺
    if (key_sub1 != key_sub2)
        return;

    greedy_match(node, vec);
}

template <typename K, typename T, typename Compare>
typename radix_tree<K, T, Compare>::iterator radix_tree<K, T, Compare>::longest_match(const K& key)
{//��ƥ���˼�����ҵ��뵱ǰ�ڵ㾡���ܶ�ģ������ַ�������ͬǰ׺�ַ��������Ҷ�
    if (m_root == NULL)
        return iterator(NULL);

    radix_tree_node<K, T, Compare>* node;
    K key_sub;

    node = find_node(key, m_root, 0);

    if (node->m_is_leaf)
        return iterator(node);

    key_sub = radix_substr(key, node->m_depth, radix_length(node->m_key));
    //����Ⱦͻ��ݵ���ĸ�ڵ�
    if (!(key_sub == node->m_key))
        node = node->m_parent;

    K nul = radix_substr(key, 0, 0);

    while (node != NULL) {
        typename radix_tree_node<K, T, Compare>::it_child it;
        it = node->m_children.find(nul);
        //����ҵ�����Ҷ�ӽڵ㣬�򷵻�
        if (it != node->m_children.end() && it->second->m_is_leaf)
            return iterator(it->second);
        //���ϻ���
        node = node->m_parent;
    }

    return iterator(NULL);
}


template <typename K, typename T, typename Compare>
typename radix_tree<K, T, Compare>::iterator radix_tree<K, T, Compare>::end()
{
    return iterator(NULL);
}

template <typename K, typename T, typename Compare>
typename radix_tree<K, T, Compare>::iterator radix_tree<K, T, Compare>::begin()
{
    radix_tree_node<K, T, Compare>* node;

    if (m_root == NULL || m_size == 0)
        node = NULL;
    else
        node = begin(m_root);

    return iterator(node);
}

template <typename K, typename T, typename Compare>
radix_tree_node<K, T, Compare>* radix_tree<K, T, Compare>::begin(radix_tree_node<K, T, Compare>* node)
{
    if (node->m_is_leaf)
        return node;


    assert(!node->m_children.empty());

    return begin(node->m_children.begin()->second);
}

template <typename K, typename T, typename Compare>
T& radix_tree<K, T, Compare>::operator[] (const K& lhs)
{
    iterator it = find(lhs);
    //�������û�иü�ֵ
    if (it == end()) {
        std::pair<K, T> val;
        val.first = lhs;

        std::pair<iterator, bool> ret;
        //����ü�ֵ���������������λ��
        ret = insert(val);

        assert(ret.second == true);

        it = ret.first;
    }

    return it->second;
}

template <typename K, typename T, typename Compare>
void radix_tree<K, T, Compare>::greedy_match(const K& key, std::vector<iterator>& vec)
{//�ҵ������뵱ǰ��ֵ��ͬ�Ĺ���ǰ׺���ַ���
    radix_tree_node<K, T, Compare>* node;

    vec.clear();
    //������ڵ�Ϊ�գ��ͷ���
    if (m_root == NULL)
        return;
    
    node = find_node(key, m_root, 0);
    //����
    if (node->m_is_leaf)
        node = node->m_parent;

    greedy_match(node, vec);
}

template <typename K, typename T, typename Compare>
void radix_tree<K, T, Compare>::greedy_match(radix_tree_node<K, T, Compare>* node, std::vector<iterator>& vec)
{
    //�ҵ�Ҷ�ӽڵ㣬��˵���÷�֧�����й���ǰ׺�ģ��������������Ҷ�ӽڵ㣬������Ҷ�ӽڵ����
    if (node->m_is_leaf) {
        vec.push_back(iterator(node));
        return;
    }

    typename std::map<K, radix_tree_node<K, T, Compare>*>::iterator it;
    //�������еĺ���
    for (it = node->m_children.begin(); it != node->m_children.end(); ++it) {
        greedy_match(it->second, vec);
    }
}

template <typename K, typename T, typename Compare>
void radix_tree<K, T, Compare>::erase(iterator it)
{
    erase(it->first);
}

template <typename K, typename T, typename Compare>
bool radix_tree<K, T, Compare>::erase(const K& key)
{
    if (m_root == NULL)
        return 0;

    radix_tree_node<K, T, Compare>* child;
    radix_tree_node<K, T, Compare>* parent;
    radix_tree_node<K, T, Compare>* grandparent;
    K nul = radix_substr(key, 0, 0);

    child = find_node(key, m_root, 0);

    if (!child->m_is_leaf)
        return 0;

    parent = child->m_parent;
    parent->m_children.erase(nul);

    delete child;

    m_size--;

    if (parent == m_root)
        return 1;

    if (parent->m_children.size() > 1)
        return 1;

    if (parent->m_children.empty()) {
        grandparent = parent->m_parent;
        grandparent->m_children.erase(parent->m_key);
        delete parent;
    }
    else {
        grandparent = parent;
    }

    if (grandparent == m_root) {
        return 1;
    }

    if (grandparent->m_children.size() == 1) {
        // merge grandparent with the uncle
        typename std::map<K, radix_tree_node<K, T, Compare>*>::iterator it;
        it = grandparent->m_children.begin();

        radix_tree_node<K, T, Compare>* uncle = it->second;

        if (uncle->m_is_leaf)
            return 1;

        uncle->m_depth = grandparent->m_depth;
        uncle->m_key = radix_join(grandparent->m_key, uncle->m_key);
        uncle->m_parent = grandparent->m_parent;

        grandparent->m_children.erase(it);

        grandparent->m_parent->m_children.erase(grandparent->m_key);
        grandparent->m_parent->m_children[uncle->m_key] = uncle;

        delete grandparent;
    }

    return 1;
}


template <typename K, typename T, typename Compare>
radix_tree_node<K, T, Compare>* radix_tree<K, T, Compare>::append(radix_tree_node<K, T, Compare>* parent, const value_type& val)
{
    //����ֵ׷���ڸ�ĸ�ڵ�ĺ��ӽڵ㣬������Ҷ�ӽڵ�ָ��
    int depth;
    int len;
    //���ÿյļ�ֵ
    K   nul = radix_substr(val.first, 0, 0);
    radix_tree_node<K, T, Compare>* node_c, * node_cc;

    depth = parent->m_depth + radix_length(parent->m_key);
    len = radix_length(val.first) - depth;

    if (len == 0) {
        //����ĸ�ڵ��Ҫ��ӵ�ֵ�������

        //ʵ�����Ǹ���ĸ�ڵ����һ��Ҷ�ӽڵ�
        node_c = new radix_tree_node<K, T, Compare>(val, m_predicate);
        //���������븸ĸ�ڵ���ͬ
        node_c->m_depth = depth;
        node_c->m_parent = parent;
        //����ֵ��Ϊ�գ���Ҷ�ӽڵ�
        node_c->m_key = nul;
        node_c->m_is_leaf = true;
        //��ĸ�ڵ�ָ���ӽڵ����������Ϊ�ýڵ�node_c
        parent->m_children[nul] = node_c;

        return node_c;
    }
    else {
        //����ĸ�ڵ��Ҫ��ӵ�ֵ���Ȳ����
        node_c = new radix_tree_node<K, T, Compare>(val, m_predicate);
        //����ֵ���븸ĸ�ڵ㲻ͬ���Ӵ���Ϊkey_sub
        K key_sub = radix_substr(val.first, depth, len);
        //��ĸ�ڵ�ָ���ӽڵ���Ӵ�����������Ϊnode_c
        parent->m_children[key_sub] = node_c;
        //node_c����������
        node_c->m_depth = depth;
        node_c->m_parent = parent;
        node_c->m_key = key_sub;
        //����node_ccΪnode_c�ĺ��ӽڵ㣬����ΪҶ�ӽڵ�
        node_cc = new radix_tree_node<K, T, Compare>(val, m_predicate);
        node_c->m_children[nul] = node_cc;

        node_cc->m_depth = depth + len;
        node_cc->m_parent = node_c;
        node_cc->m_key = nul;
        node_cc->m_is_leaf = true;

        return node_cc;
    }
}

template <typename K, typename T, typename Compare>
radix_tree_node<K, T, Compare>* radix_tree<K, T, Compare>::prepend(radix_tree_node<K, T, Compare>* node, const value_type& val)
{
    int count;
    int len1, len2;

    len1 = radix_length(node->m_key);//�ڵ��ֵ����
    len2 = radix_length(val.first) - node->m_depth;//�ڵ���ֵ���ȵĲ�ֵ
    //�жϲ�ֵ���ֽڵ���ֵ�Ƿ���ͬ�������һ����ͬ�����˳�ѭ������0��count��������ͬ����count���ܳ��ȣ���ͬ
    for (count = 0; count < len1 && count < len2; count++) {
        if (!(node->m_key[count] == val.first[count + node->m_depth]))
            break;
    }

    assert(count != 0);
    //�ڵ�ĸ�ĸ�ڵ�ָ���ӽڵ�ɾ���ڵ�ļ�ֵ
    node->m_parent->m_children.erase(node->m_key);
    //����һ���µ�node_aȥ��Žڵ���ֵ��ͬ�Ĵӣ�0��count�����ֵļ�ֵ
    radix_tree_node<K, T, Compare>* node_a = new radix_tree_node<K, T, Compare>(m_predicate);

    node_a->m_parent = node->m_parent;
    node_a->m_key = radix_substr(node->m_key, 0, count);
    node_a->m_depth = node->m_depth;
    node_a->m_parent->m_children[node_a->m_key] = node_a;

    //��node���²��� ��node�ļ�ֵ��Ϊnode�ӣ�count���ܳ��ȣ��ǲ��ֵļ�ֵ
    node->m_depth += count;
    node->m_parent = node_a;
    node->m_key = radix_substr(node->m_key, count, len1 - count);
    node->m_parent->m_children[node->m_key] = node;
    //����һ��Ҷ�ӽڵ�
    K nul = radix_substr(val.first, 0, 0);
    if (count == len2) {//Ҳ����˵��ֵһ����node��valueһģһ����׷��һ��Ҷ�ӽڵ�
        radix_tree_node<K, T, Compare>* node_b;

        node_b = new radix_tree_node<K, T, Compare>(val, m_predicate);

        node_b->m_parent = node_a;
        node_b->m_key = nul;
        node_b->m_depth = node_a->m_depth + count;
        node_b->m_is_leaf = true;
        node_b->m_parent->m_children[nul] = node_b;

        return node_b;
    }
    else {
        //node_b��� node��value��ͬ�ģ���count�������Ƕμ�ֵ
        //node_c��Ҷ�ӽڵ�
        radix_tree_node<K, T, Compare>* node_b, * node_c;
        
        node_b = new radix_tree_node<K, T, Compare>(m_predicate);

        node_b->m_parent = node_a;
        node_b->m_depth = node->m_depth;
        node_b->m_key = radix_substr(val.first, node_b->m_depth, len2 - count);
        node_b->m_parent->m_children[node_b->m_key] = node_b;

        node_c = new radix_tree_node<K, T, Compare>(val, m_predicate);

        node_c->m_parent = node_b;
        node_c->m_depth = radix_length(val.first);
        node_c->m_key = nul;
        node_c->m_is_leaf = true;
        node_c->m_parent->m_children[nul] = node_c;

        return node_c;
    }
}

template <typename K, typename T, typename Compare>
std::pair<typename radix_tree<K, T, Compare>::iterator, bool> radix_tree<K, T, Compare>::insert(const value_type& val)
{//����ֵ�������Ϣ��������λ�ã�Ҷ�ӱ�־��
    //������ڵ�Ϊ�գ�����һ��ָ��ռ�ֵ�ĸ��ڵ�
    if (m_root == NULL) {
        K nul = radix_substr(val.first, 0, 0);
        m_root = new radix_tree_node<K, T, Compare>(m_predicate);
        m_root->m_key = nul;
    }


    radix_tree_node<K, T, Compare>* node = find_node(val.first, m_root, 0);
    
    if (node->m_is_leaf) {//����ýڵ���Ҷ�ӽڵ�
        return std::pair<iterator, bool>(node, false);
    }
    else if (node == m_root) {//����ýڵ��Ǹ��ڵ㣬����ֵ׷�ӵ����ڵ�ĺ��ӽڵ���
        m_size++;
        return std::pair<iterator, bool>(append(m_root, val), true);
    }
    else {//����ýڵ㲻�Ǹ��ڵ�
        m_size++;
        int len = radix_length(node->m_key);

        K   key_sub = radix_substr(val.first, node->m_depth, len);

        if (key_sub == node->m_key) {//�Ӵ��ͽڵ��ֵ���
            return std::pair<iterator, bool>(append(node, val), true);
        }
        else {//�Ӵ��ͽڵ��ֵ�����
            return std::pair<iterator, bool>(prepend(node, val), true);
        }
    }
}

template <typename K, typename T, typename Compare>
typename radix_tree<K, T, Compare>::iterator radix_tree<K, T, Compare>::find(const K& key)
{
    if (m_root == NULL)
        return iterator(NULL);

    radix_tree_node<K, T, Compare>* node = find_node(key, m_root, 0);

    // if the node is a internal node, return NULL
    if (!node->m_is_leaf)
        return iterator(NULL);

    return iterator(node);
}

template <typename K, typename T, typename Compare>
radix_tree_node<K, T, Compare>* radix_tree<K, T, Compare>::find_node(const K& key, radix_tree_node<K, T, Compare>* node, int depth)
{//�ӵ�ǰ�ڵ�node�����Ϊdepth����ʼ���Ҽ�ֵkey�����ؽڵ�ָ��
    //���˵�ڵ��޺��ӣ����ؽڵ�
    if (node->m_children.empty())
        return node;
    //�ڵ�ĺ��ӵ�����
    typename radix_tree_node<K, T, Compare>::it_child it;
    //��ֵ����-��ǰ�����
    int len_key = radix_length(key) - depth;
    //�ӵ�ǰ��ȿ�ʼ���������к��ӽڵ�
    for (it = node->m_children.begin(); it != node->m_children.end(); ++it) {
        //�����ǰ���ʵ���ֵ�ĳ�����
        if (len_key == 0) {
            //�����ǰ���ʵĽڵ���Ҷ�ӽڵ㣬�򷵻�����ڵ㣬�������
            if (it->second->m_is_leaf)
                return it->second;
            else
                continue;
        }
        //������ʵĵ�ǰ�ڵ㲻��Ҷ�ӽڵ���ҵ�ǰ�ļ�ֵ��ֵ���ڽڵ�ļ�ֵ�ĵ�һ��ֵ��������һ�εĳ���
        if (!it->second->m_is_leaf && key[depth] == it->first[0]) {
            int len_node = radix_length(it->first);
            K   key_sub = radix_substr(key, depth, len_node);
            //���������ͬ�Ĳ���key_subʱ����ǵ���it->first������ǣ�����Ҽ�ֵ����it->second�ڵ�������depth+len_node�ڵ㣬������ǣ��ͷ��ص�ǰ��it->second�ڵ�
            if (key_sub == it->first) {
                return find_node(key, it->second, depth + len_node);
            }
            else {
                return it->second;
            }
        }
    }

    return node;
}

#endif // RADIX_TREE_HPP