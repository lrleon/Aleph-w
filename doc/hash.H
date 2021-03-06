/**
   @defgroup Hash Tablas hash
   @{ Tablas hash
   
   `Aleph-w` (\f$\aleph_\omega\f$) define la mayor�a de tipos de tablas
   hash existentes para guardar claves en memoria principal.

   Hay tres grupo de tablas hash:
   -# Con resoluci�n de colisiones por encadenamiento separado (enfoque
   tradicional) 
   @see LhashTable LhashTableVtl 
   -# Con resoluci�n de colisiones por encadenamiento separado y
   crecimiento din�mico y lineal
   @see GenLinearHashTable
   -# Resoluci�n de colisiones cerrada (open addressing)
   @see OLhashTable ODhashTable

   Todas las tablas con resoluci�n de colisi�n por encadenamiento
   separado manejan "cubetas"; es decir, su objeto de operaci�n es la
   cubeta contentiva de la clave junto con cualquier informaci�n
   asociada. Este enfoque, aunque muy versatil, es m�s engorroso que un
   simple contenedor. Sin embargo todas las clases que manejan cubetas
   tienen una derivaci�n que maneja directamente un conjunto din�mico.
   @see DynLhashTable LinearHashTable LinearHashTableVtl
   
   @author Leandro Rabindranath Le�n (lrleon en ula punto ve)
   @}
*/
