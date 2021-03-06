/**
   @defgroup Redes Redes de Flujo.
   @{ Redes de flujo

   Una red de flujo es un digrafo dirigido que abstrae una red de
   tuber�as por donde circula un fluido. Los arcos representan tuber�as;
   los nodos las junturas. Cada arco asocia dos valores num�ricos:
   asociados a   
   cada arco: 
   -# Capacidad: M�ximo valor de flujo que puede circular por el arco.
   -# Valor de flujo: Valor actual del flujo que circula por el arco.
   .

   A una red de flujo tambi�n se le conoce como red capacitada. En una
   red capacitada se identifican nodos especiales denominados "fuentes"
   y "sumideros", respectivamente. Un nodo fuente es uno que no tiene
   arcos de entrada; consecuentemente, s�lo emana flujo. An�logamente,
   un nodo sumidero es uno que no tiene arcos de salida; en el mismo
   sentido, s�lo recibe flujo.

   Puesto que las abstracciones provienen de la teor�a f�sica, para cada
   nodo que no sea fuente o sumidero siempre se debe cumplir que la
   cantidad total de flujo de entrada debe ser la misma que la de salida.

   Las redes capacitadas son un excelente veh�culo de resoluci�n de
   problemas sobre grafos. Aparte del problema del flujo m�ximo,
   representable en muchas situaciones de la vida real, una red
   capacitada sirve para resolver otros problemas de optimizaci�n
   combinatoria: caminos m�nimos, emparejamientos, cortes, etc.
   @}
   */
