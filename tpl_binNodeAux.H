# ifndef TPL_BINNODE_H
# define TPL_BINNODE_H
 
# include <ahDefs.H>
# include <ahAssert.H>
# include <ahNew.H>


# define CORPUS_BINNODE(name, height, Control_Data)                   \
    template <class Key>                                              \
class name : public Control_Data                                      \
{                                                                     \
public:                                                               \
                                                                      \
  static const size_t MaxHeight = height;                             \
                                                                      \
  static name * NullPtr;                                              \
                                                                      \
private:                                                              \
                                                                      \
  Key    key;                                                         \
  name * lLink;                                                       \
  name * rLink;                                                       \
                                                                      \
public:                                                               \
                                                                      \
  Key& get_key() { return key; }                                       \
                                                                      \
  name*& getL() { return lLink; }                                     \
                                                                      \
  name*& getR() { return rLink; }                                     \
                                                                      \
  name(const Key& k) : key(k), lLink(NullPtr), rLink(NullPtr)         \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(const Control_Data & control_data, const Key& k) :             \
    Control_Data(control_data),                                       \
    key(k), lLink(NullPtr), rLink(NullPtr)                            \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(const Control_Data & control_data) :                           \
    Control_Data(control_data),                                       \
    lLink(NullPtr), rLink(NullPtr)                                    \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name() : lLink(NullPtr), rLink(NullPtr)                             \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(SentinelCtor) : Control_Data(sentinelCtor) { /* Empty */ }     \
                                                                      \
  void reset() { rLink = lLink = NullPtr; }



# define DECLARE_BINNODE(name, height, Control_Data) \
CORPUS_BINNODE(name, height, Control_Data)           \
};                                                   \
CORPUS_BINNODE(name##Vtl, height, Control_Data)      \
virtual ~name##Vtl() { /* empty */ }                 \
}

# define SET_BINNODE_NULL_POINTER(ptr, name)                        \
template <class Key> name<Key> * name<Key>::NullPtr = ptr;          \
template <class Key> name##Vtl<Key> * name##Vtl<Key>::NullPtr = ptr


DECLARE_BINNODE(BinNode, 255, Aleph::Empty_Class);

SET_BINNODE_NULL_POINTER(nullptr, BinNode);


# endif

