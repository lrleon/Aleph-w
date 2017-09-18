
# ifndef TPL_DYNARRAY_SET_H
# define TPL_DYNARRAY_SET_H

# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>

namespace Aleph
{

    /** Conjunto de elementos implantado mediante un arreglo dinámico.
    
        DynArray_Set instrumenta un conjunto de elementos de tipo T. 

        El conjunto está internamente representado mediante un arreglo
        dinámico del tipo DynArray. Consecuentemente, el consumo en
        memoria es proporcional al número de elementos. 

        La inserción es sumamente rápida, la búsqueda es lineal y la
        eliminación de un elementpo ya encontrado también es muy rápida.

        La clase recibe dos parámetros tipo: el tipo de elemento del
        conjunto y una clase de comparación cuyo único rol es determinar
        si un elemento es igual a no a otro.

        Por razones de rapidez, se permite duplicar elementos.
        
        @see DynArray
        \ingroup Secuencias
    */
template <typename T, class Equal = Aleph::equal_to<T>>
  using DynArray_Set = DynArray<T>;

};

# endif // TPL_DYNARRAY_SET_H

