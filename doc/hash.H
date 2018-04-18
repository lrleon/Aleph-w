/**
   @defgroup Hash Tablas hash
   @{ Tablas hash
   
   `Aleph-w` (\f$\aleph_\omega\f$) define la mayoría de tipos de tablas
   hash existentes para guardar claves en memoria principal.

   Hay tres grupo de tablas hash:
   -# Con resolución de colisiones por encadenamiento separado (enfoque
   tradicional) 
   @see LhashTable LhashTableVtl 
   -# Con resolución de colisiones por encadenamiento separado y
   crecimiento dinámico y lineal
   @see GenLinearHashTable
   -# Resolución de colisiones cerrada (open addressing)
   @see OLhashTable ODhashTable

   Todas las tablas con resolución de colisión por encadenamiento
   separado manejan "cubetas"; es decir, su objeto de operación es la
   cubeta contentiva de la clave junto con cualquier información
   asociada. Este enfoque, aunque muy versatil, es más engorroso que un
   simple contenedor. Sin embargo todas las clases que manejan cubetas
   tienen una derivación que maneja directamente un conjunto dinámico.
   @see DynLhashTable LinearHashTable LinearHashTableVtl
   
   @author Leandro Rabindranath León (lrleon en ula punto ve)
   @}
*/
