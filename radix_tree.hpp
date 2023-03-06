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
{//将size_t转化为int
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
    //构造函数：用参数列表初始化成员 无参数构造
    radix_tree() : m_size(0), m_root(NULL), m_predicate(Compare()) { }
    //禁止隐式转换 有参数构造
    explicit radix_tree(Compare pred) : m_size(0), m_root(NULL), m_predicate(pred) { }
    //析构函数
    ~radix_tree() {
        delete m_root;
    }
    //m_size表示基数树上有几个字符串
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
            //将当前迭代器指向的键值赋值给toDelete
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
    //将键值追加在父母节点的后面，并返回叶子节点
    radix_tree_node<K, T, Compare>* append(radix_tree_node<K, T, Compare>* parent, const value_type& val);
    //拆分与原来节点不同的节点，并新增节点，最后返回叶子节点
    radix_tree_node<K, T, Compare>* prepend(radix_tree_node<K, T, Compare>* node, const value_type& val);
    void greedy_match(radix_tree_node<K, T, Compare>* node, std::vector<iterator>& vec);
    //复制函数和赋值函数
    radix_tree(const radix_tree& other); // delete
    radix_tree& operator =(const radix_tree other); // delete
};

template <typename K, typename T, typename Compare>
void radix_tree<K, T, Compare>::prefix_match(const K& key, std::vector<iterator>& vec)
{//前缀匹配的思想是找到所有与当前键值相同的最长字符串（短找长）
    vec.clear();
    //根节点为空，无需前缀匹配
    if (m_root == NULL)
        return;

    radix_tree_node<K, T, Compare>* node;
    K key_sub1, key_sub2;
    //从m_root根节点，深度为0开始找键值key，返回节点指针node
    node = find_node(key, m_root, 0);
    //如果该节点是叶子节点，节点指向其父母节点
    if (node->m_is_leaf)
        node = node->m_parent;
    //键值的长度-节点的深度
    int len = radix_length(key) - node->m_depth;
    //key_sub1表示键值多余不相同的部分，key_sub2表示node从node->m_key从0开始的len长度的部分
    key_sub1 = radix_substr(key, node->m_depth, len);
    key_sub2 = radix_substr(node->m_key, 0, len);
    //如果二者不相同，则无前缀，返回，否则指向贪心匹配，看是否还有更多的前缀
    if (key_sub1 != key_sub2)
        return;

    greedy_match(node, vec);
}

template <typename K, typename T, typename Compare>
typename radix_tree<K, T, Compare>::iterator radix_tree<K, T, Compare>::longest_match(const K& key)
{//长匹配的思想是找到与当前节点尽可能多的，现有字符串中相同前缀字符串，长找短
    if (m_root == NULL)
        return iterator(NULL);

    radix_tree_node<K, T, Compare>* node;
    K key_sub;

    node = find_node(key, m_root, 0);

    if (node->m_is_leaf)
        return iterator(node);

    key_sub = radix_substr(key, node->m_depth, radix_length(node->m_key));
    //不相等就回溯到父母节点
    if (!(key_sub == node->m_key))
        node = node->m_parent;

    K nul = radix_substr(key, 0, 0);

    while (node != NULL) {
        typename radix_tree_node<K, T, Compare>::it_child it;
        it = node->m_children.find(nul);
        //如果找到的是叶子节点，则返回
        if (it != node->m_children.end() && it->second->m_is_leaf)
            return iterator(it->second);
        //向上回溯
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
    //如果树中没有该键值
    if (it == end()) {
        std::pair<K, T> val;
        val.first = lhs;

        std::pair<iterator, bool> ret;
        //插入该键值，返回这个迭代器位置
        ret = insert(val);

        assert(ret.second == true);

        it = ret.first;
    }

    return it->second;
}

template <typename K, typename T, typename Compare>
void radix_tree<K, T, Compare>::greedy_match(const K& key, std::vector<iterator>& vec)
{//找到所有与当前键值相同的公共前缀的字符串
    radix_tree_node<K, T, Compare>* node;

    vec.clear();
    //如果根节点为空，就返回
    if (m_root == NULL)
        return;
    
    node = find_node(key, m_root, 0);
    //回溯
    if (node->m_is_leaf)
        node = node->m_parent;

    greedy_match(node, vec);
}

template <typename K, typename T, typename Compare>
void radix_tree<K, T, Compare>::greedy_match(radix_tree_node<K, T, Compare>* node, std::vector<iterator>& vec)
{
    //找到叶子节点，则说明该分支是与有公共前缀的，迭代器将加入该叶子节点，把所有叶子节点加入
    if (node->m_is_leaf) {
        vec.push_back(iterator(node));
        return;
    }

    typename std::map<K, radix_tree_node<K, T, Compare>*>::iterator it;
    //遍历所有的孩子
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
    //将键值追加在父母节点的孩子节点，并返回叶子节点指针
    int depth;
    int len;
    //设置空的键值
    K   nul = radix_substr(val.first, 0, 0);
    radix_tree_node<K, T, Compare>* node_c, * node_cc;

    depth = parent->m_depth + radix_length(parent->m_key);
    len = radix_length(val.first) - depth;

    if (len == 0) {
        //当父母节点和要添加的值长度相等

        //实际上是给父母节点添加一个叶子节点
        node_c = new radix_tree_node<K, T, Compare>(val, m_predicate);
        //设置属性与父母节点相同
        node_c->m_depth = depth;
        node_c->m_parent = parent;
        //将键值设为空，是叶子节点
        node_c->m_key = nul;
        node_c->m_is_leaf = true;
        //父母节点指向孩子节点的数组内容为该节点node_c
        parent->m_children[nul] = node_c;

        return node_c;
    }
    else {
        //当父母节点和要添加的值长度不相等
        node_c = new radix_tree_node<K, T, Compare>(val, m_predicate);
        //将键值的与父母节点不同的子串设为key_sub
        K key_sub = radix_substr(val.first, depth, len);
        //父母节点指向孩子节点的子串的数组内容为node_c
        parent->m_children[key_sub] = node_c;
        //node_c的属性设置
        node_c->m_depth = depth;
        node_c->m_parent = parent;
        node_c->m_key = key_sub;
        //设置node_cc为node_c的孩子节点，且设为叶子节点
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

    len1 = radix_length(node->m_key);//节点键值长度
    len2 = radix_length(val.first) - node->m_depth;//节点与值长度的差值
    //判断差值部分节点与值是否相同，如果有一个不同，则退出循环，（0，count）部分相同，（count，总长度）不同
    for (count = 0; count < len1 && count < len2; count++) {
        if (!(node->m_key[count] == val.first[count + node->m_depth]))
            break;
    }

    assert(count != 0);
    //节点的父母节点指向孩子节点删除节点的键值
    node->m_parent->m_children.erase(node->m_key);
    //设置一个新的node_a去存放节点与值相同的从（0，count）部分的键值
    radix_tree_node<K, T, Compare>* node_a = new radix_tree_node<K, T, Compare>(m_predicate);

    node_a->m_parent = node->m_parent;
    node_a->m_key = radix_substr(node->m_key, 0, count);
    node_a->m_depth = node->m_depth;
    node_a->m_parent->m_children[node_a->m_key] = node_a;

    //对node重新操作 将node的键值设为node从（count，总长度）那部分的键值
    node->m_depth += count;
    node->m_parent = node_a;
    node->m_key = radix_substr(node->m_key, count, len1 - count);
    node->m_parent->m_children[node->m_key] = node;
    //设置一个叶子节点
    K nul = radix_substr(val.first, 0, 0);
    if (count == len2) {//也就是说差值一样，node和value一模一样，追加一个叶子节点
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
        //node_b存放 node与value不同的，从count到最后的那段键值
        //node_c是叶子节点
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
{//返回值的相关信息（迭代器位置，叶子标志）
    //如果根节点为空，创造一个指向空键值的根节点
    if (m_root == NULL) {
        K nul = radix_substr(val.first, 0, 0);
        m_root = new radix_tree_node<K, T, Compare>(m_predicate);
        m_root->m_key = nul;
    }


    radix_tree_node<K, T, Compare>* node = find_node(val.first, m_root, 0);
    
    if (node->m_is_leaf) {//如果该节点是叶子节点
        return std::pair<iterator, bool>(node, false);
    }
    else if (node == m_root) {//如果该节点是根节点，将键值追加到根节点的孩子节点上
        m_size++;
        return std::pair<iterator, bool>(append(m_root, val), true);
    }
    else {//如果该节点不是根节点
        m_size++;
        int len = radix_length(node->m_key);

        K   key_sub = radix_substr(val.first, node->m_depth, len);

        if (key_sub == node->m_key) {//子串和节点键值相等
            return std::pair<iterator, bool>(append(node, val), true);
        }
        else {//子串和节点键值不相等
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
{//从当前节点node，深度为depth，开始查找键值key，返回节点指针
    //如果说节点无孩子，返回节点
    if (node->m_children.empty())
        return node;
    //节点的孩子迭代器
    typename radix_tree_node<K, T, Compare>::it_child it;
    //键值长度-当前的深度
    int len_key = radix_length(key) - depth;
    //从当前深度开始，遍历所有孩子节点
    for (it = node->m_children.begin(); it != node->m_children.end(); ++it) {
        //如果当前访问到键值的长度了
        if (len_key == 0) {
            //如果当前访问的节点是叶子节点，则返回这个节点，否则继续
            if (it->second->m_is_leaf)
                return it->second;
            else
                continue;
        }
        //如果访问的当前节点不是叶子节点而且当前的键值的值等于节点的键值的第一个值，再找下一段的长度
        if (!it->second->m_is_leaf && key[depth] == it->first[0]) {
            int len_node = radix_length(it->first);
            K   key_sub = radix_substr(key, depth, len_node);
            //继续检查相同的部分key_sub时候就是等于it->first，如果是，则查找键值，从it->second节点往下找depth+len_node节点，如果不是，就返回当前的it->second节点
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