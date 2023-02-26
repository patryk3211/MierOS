#pragma once

#include <optional.hpp>
#include <allocator.hpp>
#include <pair.hpp>

#include <dmesg.h>
#include <string.hpp>
#include <printf.h>

namespace std {
    template<typename K, typename V, class Allocator = std::heap_allocator> class RangeMap {
        struct Node {
            Node* f_parent;
            Node* f_child[2];
            enum Color {
                Black = 0,
                Red = 1
            } f_color;

            const K f_key;
            alignas(V) unsigned char f_storage[sizeof(V)];

            Node(const K& key)
                : f_key(key) {
                new(f_storage) V();
                f_parent = 0;
                f_child[0] = 0;
                f_child[1] = 0;
                f_color = Black;
            }

            Node(const K& key, const V& value)
                : f_key(key) {
                new(f_storage) V(value);
                f_parent = 0;
                f_child[0] = 0;
                f_child[1] = 0;
                f_color = Black;
            }

            Node(const K& key, V&& value)
                : f_key(key) {
                new(f_storage) V(value);
                f_parent = 0;
                f_child[0] = 0;
                f_child[1] = 0;
                f_color = Black;
            }

            ~Node() {
                value().~V();
            }

            V& value() {
                return *reinterpret_cast<V*>(f_storage);
            }
        };

        static constexpr int Left = 0;
        static constexpr int Right = 1;

        Node* f_root;

        Allocator alloc;

    public:
        class iterator {
            Node* f_current_node;
            int f_step;

            iterator(Node* node, int step)
                : f_current_node(node), f_step(step) { }

            Node* get_next_node(Node* current) {
                if(f_step == 0) {
                    // Step = 0, moving up
                    if(current->f_parent->f_child[Left] == current) {
                        // Left child, next entry is parent,
                        // next step is to fall down to the right
                        f_step = 1;
                        return current->f_parent;
                    } else {
                        // Right child, we move up until we are not
                        // the right child.
                        Node* last = f_current_node;
                        while(last->f_parent && last->f_parent->f_child[Right] == last)
                            last = last->f_parent;

                        f_step = 1;
                        return last->f_parent;
                    }
                } else if(f_step == 1) {
                    // Step = 1, falling down
                    Node* last = f_current_node->f_child[Right];
                    if(!last) {
                        // No right child, return parent,
                        // Next step is still to fall down.
                        return f_current_node->f_parent;
                    }

                    while(last->f_child[Left]) last = last->f_child[Left];
                    // Return left most child,
                    // next step is to go up
                    f_step = 0;
                    return last;
                } else {
                    panic("Invalid step");
                }
            }
        public:

            ~iterator() = default;

            iterator& operator++() {
                f_current_node = get_next_node(f_current_node);
                return *this;
            }

            iterator operator++(int) {
                iterator prev(f_current_node, f_step);
                f_current_node = get_next_node(f_current_node);
                return prev;
            }

            Pair<const K&, V&> operator*() {
                return Pair<const K&, V&>(f_current_node->f_key, f_current_node->value());
            }

            bool operator==(const iterator& other) {
                return f_current_node == other.f_current_node;
            }

            bool operator!=(const iterator& other) {
                return !(*this == other);
            }

            friend class RangeMap;
        };

    private:
        void delete_node(Node* node) {
            if(!node)
                return;
            if(node->f_child[0])
                delete_node(node->f_child[0]);
            if(node->f_child[1])
                delete_node(node->f_child[1]);
            alloc.free(node);
        }

        Node* rotate_node(Node* subtree, int direction) {
            Node* parent = subtree->f_parent;
            Node* opposite = subtree->f_child[1 - direction];
            if(!opposite)
                panic("invalid");
            Node* child = opposite->f_child[direction];
            subtree->f_child[1 - direction] = child;
            if(child)
                child->f_parent = subtree;
            opposite->f_child[direction] = subtree;
            subtree->f_parent = opposite;
            opposite->f_parent = parent;
            if(parent)
                parent->f_child[subtree == parent->f_child[1]] = opposite;
            else
                f_root = opposite;
            return opposite;
        }

        void insert_node(Node* parent, Node* node, int direction) {
            node->f_color = Node::Red;
            node->f_child[0] = 0;
            node->f_child[1] = 0;
            node->f_parent = parent;
            if(!parent) {
                f_root = node;
                return;
            }
            parent->f_child[direction] = node;

            Node* grandparent;
            do {
                if(parent->f_color == Node::Black) {
                    // Case I1
                    return;
                }
                // parent is red
                if(!(grandparent = parent->f_parent)) {
                    // Case I4
                    // Parent is root so we can just recolor the parent
                    parent->f_color = Node::Black;
                    return;
                }
                // Parent is not root and is red
                direction = grandparent->f_child[Right] == parent; // Is parent the right or left child of grandparent
                Node* uncle = grandparent->f_child[1 - direction];
                if(!uncle || uncle->f_color == Node::Black) {
                    // Case I56
                    if(node == parent->f_child[1 - direction]) {
                        rotate_node(parent, direction);
                        node = parent;
                        parent = grandparent->f_child[direction];
                    }
                    rotate_node(grandparent, 1 - direction);
                    parent->f_color = Node::Black;
                    grandparent->f_color = Node::Red;
                    return;
                }

                // Case I2
                parent->f_color = Node::Black;
                uncle->f_color = Node::Black;
                grandparent->f_color = Node::Red;

                node = parent->f_parent;
            } while((parent = node->f_parent));
            // Case I3, leave the loop
        }

        void copy_node(Node* source, Node** dest) {
            *dest = alloc.template alloc<Node>(source->f_key, source->value());

            if(source->f_child[0])
                copy_node(source->f_child[0], &(*dest)->f_child[0]);
            if(source->f_child[1])
                copy_node(source->f_child[1], &(*dest)->f_child[1]);
        }
    public:
        RangeMap() {
            f_root = 0;
        }

        RangeMap(const RangeMap& other) {
            if(!other.f_root) {
                f_root = 0;
                return;
            }

            copy_node(other.f_root, &f_root);
        }

        RangeMap& operator=(const RangeMap& other) {
            delete_node(f_root);
            
            if(!other.f_root) {
                f_root = 0;
                return *this;
            }

            copy_node(other.f_root, &f_root);
            return *this;
        }

        ~RangeMap() {
            delete_node(f_root);
        }

        std::OptionalRef<V> at(const K& key) {
            Node* currentRoot = f_root;
            if(currentRoot == 0)
                return { };
            
            while(currentRoot) {
                if(currentRoot->f_key == key)
                    return currentRoot->value();
                if(key < currentRoot->f_key) {
                    currentRoot = currentRoot->f_child[Left];
                } else {
                    currentRoot = currentRoot->f_child[Right];
                }
            }

            return { };
        }

        V& operator[](const K& key) {
            Node* currentRoot = f_root;
            if(currentRoot == 0) {
                f_root = alloc.template alloc<Node>(key);
                return f_root->value();
            }

            Node* prev = 0;
            while(currentRoot) {
                if(currentRoot->f_key == key)
                    return currentRoot->value();

                prev = currentRoot;
                if(key < currentRoot->f_key) {
                    currentRoot = currentRoot->f_child[Left];
                } else {
                    currentRoot = currentRoot->f_child[Right];
                }
            }

            Node* newNode = alloc.template alloc<Node>(key);
            int dir = key < prev->f_key ? Left : Right;
            insert_node(prev, newNode, dir);
            return newNode->value();
        }

        std::OptionalRef<V> at_range(const K& key) {
            Node* currentRoot = f_root;
            if(currentRoot == 0)
                return { };
            
            Node* lastValid = 0;
            while(currentRoot) {
                if(currentRoot->f_key == key)
                    return currentRoot->value();
                if(key < currentRoot->f_key) {
                    currentRoot = currentRoot->f_child[Left];
                } else {
                    lastValid = currentRoot;
                    currentRoot = currentRoot->f_child[Right];
                }
            }

            if(lastValid)
                return lastValid->value();
            else
                return { };
        }

        bool insert(std::Pair<const K&, const V&> pair) {
            Node* currentRoot = f_root;
            if(currentRoot == 0) {
                Node* newNode = alloc.template alloc<Node>(pair.key, pair.value);
                f_root = newNode;
                return true;
            }

            Node* prev = 0;
            while(currentRoot) {
                if(currentRoot->f_key == pair.key)
                    return false;

                prev = currentRoot;
                if(pair.key < currentRoot->f_key)
                    currentRoot = currentRoot->f_child[Left];
                else
                    currentRoot = currentRoot->f_child[Right];
            }

            Node* newNode = alloc.template alloc<Node>(pair.key, pair.value);
            int dir = pair.key < prev->f_key ? Left : Right;
            insert_node(prev, newNode, dir);
            return true;
        }

        void display(Node* node = 0, int depth = 0) {
            if(!node && !depth)
                node = f_root;

            std::String<> line;
            for(int i = 0; i < depth; ++i)
                line += "-";

            if(!node) {
                line += "null";
                dmesg(line.c_str());
                return;
            }

            if(node->f_color == Node::Black)
                line += "B ";
            else
                line += "R ";

            char buffer[40];
            sprintf(buffer, "0x%016lx -> %d", node->f_key, node->value());
            line += buffer;

            display(node->f_child[Left], depth + 1);
            dmesg(line.c_str());
            display(node->f_child[Right], depth + 1);
        }

        void clear() {
            delete_node(f_root);
            f_root = 0;
        }

        iterator begin() {
            Node* last = f_root;
            if(last == 0)
                return end();
            
            while(last->f_child[Left]) last = last->f_child[Left];
            return iterator(last, 0);
        }

        iterator end() {
            return iterator(0, 0);
        }
    };
}

