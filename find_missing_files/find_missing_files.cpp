#include "alfe/main.h"
#include "alfe/sha256.h"

//class HashSet : Uncopyable
//{
//public:
//    HashSet() { }
//
//    void insert(SHA256Hash hash)
//    {
//        Node* n = &_root;
//        for (int i = 0; i < 256; ++i) {
//            Item** item;
//            if (!hash.bit(i))
//                item = &n->_left;
//            else
//                item = &n->_right;
//            if (*item == 0) {
//                *item = new Entry(hash);
//                break;
//            }
//            n = dynamic_cast<Node*>(*item);
//            if (n != 0)
//                continue;
//            Entry* e = dynamic_cast<Entry*>(*item);
//            if (e->_hash == hash) {
//                // This means that we have two files with the same contents in
//                // the same directory, if not hashing filenames.
//                // If we are hashing filenames, this should never happen.
//                throw Exception();
//                //break;
//            }
//            n = new Node();
//            if (!e->_hash.bit(i + 1)) {
//                n->_left = e;
//                n->_right = 0;
//            }
//            else {
//                n->_left = 0;
//                n->_right = e;
//            }
//            *item = n;
//        }
//    }
//    SHA256Hash hash()
//    {
//        SHA256Hash::Hasher hasher;
//        _root.hash(&hasher);
//        return SHA256Hash(hasher);
//    }
//private:
//    struct Item
//    {
//        virtual ~Item() { };
//        virtual void hash(SHA256Hash::Hasher* hasher) = 0;
//    };
//    struct Entry : public Item
//    {
//        Entry(SHA256Hash hash) : _hash(hash) { }
//        void hash(SHA256Hash::Hasher* hasher)
//        {
//            hasher->update(_hash.data(), 32);
//        }
//        SHA256Hash _hash;
//    };
//    struct Node : public Item
//    {
//        Node() : _left(0), _right(0) { }
//        ~Node()
//        {
//            if (_left != 0)
//                delete _left;
//            if (_right != 0)
//                delete _right;
//        }
//        void hash(SHA256Hash::Hasher* hasher)
//        {
//            if (_left != 0)
//                _left->hash(hasher);
//            if (_right != 0)
//                _right->hash(hasher);
//        }
//        Item* _left;
//        Item* _right;
//    };
//
//    Node _root;
//};

class FindMissingFiles : Uncopyable
{
    struct ObjectData
    {
        ObjectData(SHA256Hash hash, Int64 size) : _hash(hash), _size(size) { }
        SHA256Hash _hash;
        Int64 _size;
    };
    //struct DirectoryProcessor
    //{
    //    DirectoryProcessor(FindMissingFiles* map) : _map(map), _size(0) { }
    //    void operator()(const File& file)
    //    {
    //        update((*_map)(file), file.name());
    //    }
    //    void operator()(const Directory& directory)
    //    {
    //        update((*_map)(directory), directory.name());
    //    }
    //    void update(ObjectData d, String name)
    //    {
    //        SHA256Hash::Hasher hasher;
    //        hasher.update(d._hash.data(), 32);
    //        hasher.update(&name[0], name.length());
    //        _set.insert(SHA256Hash(hasher));
    //        _size += d._size;
    //    }
    //    FindMissingFiles* _map;
    //    HashSet _set;
    //    Int64 _size;
    //};

    struct ArrayEntry;
    struct Item
    {
        virtual ~Item() { };
        virtual ArrayEntry* enumerate(ArrayEntry* entry) = 0;
        virtual void dump(int n) = 0;
    };
    struct Entry
    {
        Entry(FileSystemObject object)
            : _object(object), _next(0) { }
        //~Entry()
        //{
        //    if (_next != 0)
        //        delete _next;
        //}
        void print()
        {
            //if (_next != 0)
            //    _next->print();
            console.write("        " + _object.path() + "\n");
        }
        void dump(int n)
        {
            console.write(String(" ") * (n + 50) + _object.path() + "\n");
            if (_next != 0)
                _next->dump(n);
        }
        FileSystemObject _object;
        Entry* _next;
    };
    struct EntrySet : public Item
    {
        EntrySet(ObjectData data, FileSystemObject object)
            : _data(data), _first(object) { }
        ~EntrySet()
        {
            Entry* e = _first._next;
            while (e != 0) {
                Entry* n = e->_next;
                delete e;
                e = n;
            }
        }
        ArrayEntry* enumerate(ArrayEntry* entry)
        {
            Entry* e = _first._next;
            if (e == 0)
                return entry;  // Not a duplicate
            int i = 1;
            while (e != 0) {
                ++i;
                e = e->_next;
            }
            entry->_size = i * _data._size;
            entry->_entrySet = this;
            return entry + 1;
        }
        //void print() { _first._next->print(); }
        void print()
        {
            Entry* e = _first._next;
            while (e != 0) {
                e->print();
                e = e->_next;
            }
        }
        void dump(int n)
        {
            console.write(String(" ") * n + format("%016llx ", _data._size) + _data._hash.toString() + "\n");
            _first.dump(n);
        }
        ObjectData _data;
        Entry _first;
    };
    struct Node : public Item
    {
        Node() : _left(0), _right(0) { }
        ~Node()
        {
            if (_left != 0)
                delete _left;
            if (_right != 0)
                delete _right;
        }
        ArrayEntry* enumerate(ArrayEntry* entry)
        {
            if (_left != 0)
                entry = _left->enumerate(entry);
            if (_right != 0)
                entry = _right->enumerate(entry);
            return entry;
        }
        void dump(int n)
        {
            if (_left != 0)
                _left->dump(n + 1);
            if (_right != 0)
                _right->dump(n + 1);
        }
        Item* _left;
        Item* _right;
    };

    void insert(ObjectData data, FileSystemObject object)
    {
        Node* n = &_root;
        for (int i = 0; i < 256; ++i) {
            Item** item;
            if (!data._hash.bit(i))
                item = &n->_left;
            else
                item = &n->_right;
            if (*item == 0) {
                *item = new EntrySet(data, object);
                break;
            }
            n = dynamic_cast<Node*>(*item);
            if (n != 0)
                continue;
            EntrySet* e = dynamic_cast<EntrySet*>(*item);
            if (e->_data._hash == data._hash) {
                Entry* en = new Entry(object);
                en->_next = e->_first._next;
                if (en->_next == 0)
                    ++_duplicates;
                e->_first._next = en;
                break;
            }
            n = new Node();
            if (!e->_data._hash.bit(i + 1)) {
                n->_left = e;
                n->_right = 0;
            }
            else {
                n->_left = 0;
                n->_right = e;
            }
            *item = n;
        }
    }

    bool find(ObjectData data)
    {
        Node* n = &_root;
        for (int i = 0; i < 256; ++i) {
            Item** item;
            if (!data._hash.bit(i))
                item = &n->_left;
            else
                item = &n->_right;
            if (*item == 0)
                return false;
            n = dynamic_cast<Node*>(*item);
            if (n != 0)
                continue;
            EntrySet* e = dynamic_cast<EntrySet*>(*item);
            return (e->_data._hash == data._hash);
        }
        return false;
    }

    struct ArrayEntry
    {
        Int64 _size;
        EntrySet* _entrySet;

        bool operator<(const ArrayEntry& other) const
        {
            // Sort by size descending
            return _size > other._size;
        }

        void print()
        {
            String ss;
            do {
                if (_size < 1000) {
                    ss = format("  %3iB  ", _size);
                    break;
                }
                double s = _size / 1024.0;
                if (s < 1000) {
                    ss = format("%#5.3gKB ", s);
                    break;
                }
                s /= 1024.0;
                if (s < 1000) {
                    ss = format("%#5.3gMB ", s);
                    break;
                }
                s /= 1024.0;
                if (s < 1000) {
                    ss = format("%#5.3gGB ", s);
                    break;
                }
                s /= 1024.0;
                ss = format("%#5.3gTB ", s);
            } while (false);
            console.write(ss + _entrySet->_first._object.path() + "\n");
            _entrySet->print();
        }
    };
public:
    FindMissingFiles() : _duplicates(0), _phase2(false) { }
    void operator()(const File& file)
    {
        auto h = file.tryOpenRead();
        Int64 size = 0;
        if (h.valid())
            size = h.size();
        ObjectData data(SHA256Hash(file), size);
        if (!_phase2)
            insert(data, file);
        else {
            if (!find(data))
                console.write(file.path() + "\n");
        }
        //console.write(data._hash.toString() + " " + file.path() + "\n");
        //return data;
    }
    void operator()(const Directory& directory)
    {
        //DirectoryProcessor p(this);
        //try {
        //    directory.applyToContents(p, false);
        //}
        //catch (...) {

        //}
        //SHA256Hash hash(p._set.hash());
        //ObjectData data(SHA256Hash(p._set.hash()), p._size);
        //insert(data, directory);
        ////console.write(data._hash.toString() + " " + directory.path() + "\n");
        //return data;
    }
    void printDuplicates()
    {
        Array<ArrayEntry> entries(_duplicates);
        _root.enumerate(&entries[0]);
        std::sort(entries.begin(), entries.end());
        for (auto e : entries)
            e.print();
    }
    void dump() { _root.dump(0); }
    void setPhase2() { _phase2 = true; }
private:
    int _duplicates;
    Node _root;
    bool _phase2;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        // Find any files in path given in the second argument that aren't in
        // the directory tree given by the first argument

        FindMissingFiles map;
        if (_arguments.count() < 3) {
            console.write("Usage: find_missing_files <path 1> <path 2>\n");
            return;
        }
        for (int i = 1; i < _arguments.count(); ++i) {
            applyToWildcard(map, _arguments[i]);
            map.setPhase2();
        }
    }
};
