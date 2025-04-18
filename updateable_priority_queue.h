#ifndef UPDATEABLE_PRIORITY_QUEUE_H
#define UPDATEABLE_PRIORITY_QUEUE_H
#include <utility>
#include <vector>
#include <queue>
#include <iostream>
#include <algorithm>
#include <ranges>

namespace better_priority_queue
{
    template <typename Key, typename Priority>
    struct priority_queue_node
    {
        Priority priority;
        Key key;
        priority_queue_node(const Key &key, const Priority &priority) : priority(priority), key(key) {}
        friend bool operator<(const priority_queue_node &pqn1, const priority_queue_node &pqn2)
        {
            if (pqn1.priority == pqn2.priority) {
			    return pqn1.key > pqn2.key; 
		    } else {
			    return pqn1.priority < pqn2.priority;
            //return pqn1.priority < pqn2.priority;
            }
        }
        friend bool operator>(const priority_queue_node &pqn1, const priority_queue_node &pqn2)
        {
            if (pqn1.priority == pqn2.priority) {
			    return pqn1.key < pqn2.key; 
		    } else {
			    return pqn1.priority > pqn2.priority;
            //return pqn1.priority > pqn2.priority;
            }
        }
    };

    /** Key has to be an uint value (convertible to size_t)
     * This is a max heap (max is on top), to match stl's pQ */
    template <typename Key, typename Priority>
    class updatable_priority_queue
    {
    protected:
        std::vector<size_t> id_to_heappos;
        std::vector<priority_queue_node<Key, Priority>> heap;
        std::priority_queue<priority_queue_node<int,int>> topk_queue;      
        

    public:
        updatable_priority_queue() {}

        bool empty() const { return heap.empty(); }
        std::size_t size() const { return heap.size(); }

        /** first is priority, second is key */
        const priority_queue_node<Key, Priority> &top() const
        {
            return heap.front();
        }

        std::vector<Key> topk(int k) {
            //This is slower than pop k push k
            std::vector<Key> topk(k, -1);
            
            /*updatable_priority_queue<int, int>  topkqueue = updatable_priority_queue<int, int>();
            //topkqueue.extend_ids(id_to_heappos.size()) //Not needed by reduces L1 Miss
            topkqueue.push(heap[0].key, heap[0].priority);

            for (int val=0; val < k; val++) {
                topk[val] = topkqueue.top().key;
                topkqueue.pop();
                
                size_t child_pos = id_to_heappos[topk[val]] * 2 + 1;
                if (!(child_pos < id_to_heappos.size())) {
                    continue;
                } 
                topkqueue.push(heap[child_pos].key, heap[child_pos].priority);

                if (!(child_pos+1 < id_to_heappos.size())) {
                    continue;
                } 
                topkqueue.push(heap[child_pos+1].key, heap[child_pos+1].priority);
            }*/
            

            for (int val=0; val < k; val++) {
                topk[val] = heap[val].key;
            }

            return topk;
        } 

        std::vector<Key> topk_lazy(int k) {
            //This is slower than pop k push k
            std::vector<Key> topk(k, -1);
            //updatable_priority_queue<int, int>  topkqueue = updatable_priority_queue<int, int>();
            //topkqueue.extend_ids(id_to_heappos.size()) //Not needed by reduces L1 Miss
            //uniform = boost::random::uniform_int_distribution<>(0,heap.size()-1);
            topk_queue = std::priority_queue<priority_queue_node<int,int>>();
            topk_queue.push(heap[0]);
            for (int val=0; val < k; val++) {
                topk[val] = topk_queue.top().key;
                topk_queue.pop();
                
                size_t child_pos = id_to_heappos[topk[val]] * 2 + 1;

                //Look ood but saves some branching
                if ((child_pos+1 < id_to_heappos.size())) {
                    topk_queue.push(heap[child_pos]);
                    topk_queue.push(heap[child_pos+1]);
                    
                } else if ((child_pos < id_to_heappos.size())) {
                    topk_queue.push(heap[child_pos]);
                } 
            }

            //for (int val=0; val < k; val++) {
            //    topk[val] = heap[val].key;
            //}

            return topk;
        } 

        void pop(bool remember_key = false)
        {
            if (size() == 0)
                return;
            id_to_heappos[heap.front().key] = -1 - remember_key;
            if (size() > 1)
            {
                *heap.begin() = std::move(*(heap.end() - 1));
                id_to_heappos[heap.front().key] = 0;
            }
            heap.pop_back();
            sift_down(0);
        }

        priority_queue_node<Key, Priority> pop_value(bool remember_key = true)
        {
            if (size() == 0)
                return priority_queue_node<Key, Priority>(-1, Priority());
            priority_queue_node<Key, Priority> ret = std::move(*heap.begin());
            id_to_heappos[ret.key] = -1 - remember_key;
            if (size() > 1)
            {
                *heap.begin() = std::move(*(heap.end() - 1));
                id_to_heappos[heap.front().key] = 0;
            }
            heap.pop_back();
            sift_down(0);
            return ret;
        }

        /** Sets the priority for the given key. If not present, it will be added, otherwise it will be updated
         *  Returns true if the priority was changed.
         * */
        bool set(const Key &key, const Priority &priority, bool only_if_higher = false)
        {
            if (key < id_to_heappos.size() && id_to_heappos[key] < ((size_t)-2)) // This key is already in the pQ
                return update(key, priority, only_if_higher);
            else
                return push(key, priority, only_if_higher);
        }

        std::pair<bool, Priority> get_priority(const Key &key)
        {
            if (key < id_to_heappos.size())
            {
                size_t pos = id_to_heappos[key];
                if (pos < ((size_t)-2))
                {
                    return {true, heap[pos].priority};
                }
            }
            return {false, 0};
        }

        /** Returns true if the key was not inside and was added, otherwise does nothing and returns false
         *  If the key was remembered and only_if_unknown is true, does nothing and returns false
         * */
        bool push(const Key &key, const Priority &priority, bool only_if_unknown = false)
        {
            extend_ids(key);
            if (id_to_heappos[key] < ((size_t)-2))
                return false;
            if (only_if_unknown && id_to_heappos[key] == ((size_t)-2))
                return false;
            // otherwise we have id_to_heappos[key] = -1, unseen key
            size_t n = heap.size();
            id_to_heappos[key] = n; // For consistency in the case where nothing moves (early return)
            heap.emplace_back(key, priority);
            sift_up(n);
            return true;
        }

        /** Returns true if the key was already inside and was updated, otherwise does nothing and returns false */
        bool update(const Key &key, const Priority &new_priority, bool only_if_higher = false)
        {
            if (key >= id_to_heappos.size())
                return false;
            size_t heappos = id_to_heappos[key];
            if (heappos >= ((size_t)-2))
                return false;
            Priority &priority = heap[heappos].priority;
            if (new_priority > priority)
            {
                priority = new_priority;
                sift_up(heappos);
                return true;
            }
            else if (!only_if_higher && new_priority < priority)
            {
                priority = new_priority;
                sift_down(heappos);
                return true;
            }
            return false;
        }

        /** Returns true if the key was already inside and was updated, otherwise does nothing and returns false */
        bool update_scramble(const Key &key, const Priority &new_priority, Key scramble, bool only_if_higher = false)
        {
            //scramble = key;
            if (key >= id_to_heappos.size() || scramble >= id_to_heappos.size())
                return false;
            size_t heappos = id_to_heappos[key];
            size_t scramble_pos = 0;
            //std::cout << std::endl << 
            if (heappos * 2 + 3 >= id_to_heappos[key]) {
                scramble_pos = id_to_heappos[scramble];
            } else {
                scramble_pos = heappos * 2 + 3;
            }

            if (heappos >= ((size_t)-2))
                return false;

            std::swap(id_to_heappos[key], id_to_heappos[scramble]);
            std::swap(heap[id_to_heappos[key]], heap[id_to_heappos[scramble]]);
            Priority &priority = heap[heappos].priority; //Old scramble prio
            Priority &scramble_priority = heap[scramble_pos].priority; //old key prio

            //sift_up(id_to_heappos[key]);


            //sift_down(id_to_heappos[scramble]);
            //sift_up(id_to_heappos[scramble]);
            if (new_priority > priority)
            {
                scramble_priority = new_priority;
                sift_up(scramble_pos);
                return true;
            }
            else if (!only_if_higher && new_priority < priority)
            {
                scramble_priority = new_priority;
                sift_down(scramble_pos);
                return true;
            }

            if (priority > scramble_priority)
            {
                //scramble_priority = new_priority;
                sift_up(heappos);
                return true;
            }
            else if (!only_if_higher && priority < scramble_priority)
            {
                //scramble_priority = new_priority;
                sift_down(heappos);
                return true;
            }

            return true;
        }

        void normalize(const Priority &factor) {
            for (auto &f : heap) {
                f.priority /= factor;
            }
        }
        double heap_sum() {
            double sum = 0;
            for (auto v : heap) {
                v.priority += sum;
            }
            return sum;
        }

    private:
        void extend_ids(Key k)
        {
            size_t new_size = k + 1;
            if (id_to_heappos.size() < new_size)
                id_to_heappos.resize(new_size, -1);
        }

        void sift_down(size_t heappos)
        {
            size_t len = heap.size();
            size_t child = heappos * 2 + 1;
            if (len < 2 || child >= len)
                return;
            if (child + 1 < len && heap[child + 1] > heap[child])
                ++child; // Check whether second child is higher
            if (!(heap[child] > heap[heappos]))
                return; // Already in heap order

            priority_queue_node<Key, Priority> val = std::move(heap[heappos]);
            do
            {
                heap[heappos] = std::move(heap[child]);
                id_to_heappos[heap[heappos].key] = heappos;
                heappos = child;
                child = 2 * child + 1;
                if (child >= len)
                    break;
                if (child + 1 < len && heap[child + 1] > heap[child])
                    ++child;
            } while (heap[child] > val);
            heap[heappos] = std::move(val);
            id_to_heappos[heap[heappos].key] = heappos;
        }

        void sift_up(size_t heappos)
        {
            size_t len = heap.size();
            if (len < 2 || heappos <= 0)
                return;
            size_t parent = (heappos - 1) / 2;
            if (!(heap[heappos] > heap[parent]))
                return;
            priority_queue_node<Key, Priority> val = std::move(heap[heappos]);
            do
            {
                heap[heappos] = std::move(heap[parent]);
                id_to_heappos[heap[heappos].key] = heappos;
                heappos = parent;
                if (heappos <= 0)
                    break;
                parent = (parent - 1) / 2;
            } while (val > heap[parent]);
            heap[heappos] = std::move(val);
            id_to_heappos[heap[heappos].key] = heappos;
        }
    };
}
#endif //UPDATEABLE_PRIORITY_QUEUE_H
