
# ifndef TPL_RBNODE_H
# define TPL_RBNODE_H


# include <tpl_binNode.H>


# define Color        unsigned char


# ifdef DEBUG
# define RED          ('R')
# define BLACK        ('B')
# else
# define RED          (0)
# define BLACK        (1)
# endif


class RbNode_Data
{
private:
  
  Color  color; // color is either BLACK or RED 
public:

  RbNode_Data() noexcept : color(RED) { /* empty */ }

  RbNode_Data(SentinelCtor) noexcept : color(BLACK) { /* empty */ }

  Color& getColor() noexcept { return color; }

  void reset() noexcept { color = RED; }
}; 


DECLARE_BINNODE_SENTINEL(RbNode, 128, RbNode_Data);


# endif // TPL_RBNODE_H
