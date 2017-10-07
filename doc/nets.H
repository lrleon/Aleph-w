/**
   @defgroup Redes Redes de Flujo.
   @{ Redes de flujo

   Una red de flujo es un digrafo dirigido que abstrae una red de
   tuberías por donde circula un fluido. Los arcos representan tuberías;
   los nodos las junturas. Cada arco asocia dos valores numéricos:
   asociados a   
   cada arco: 
   -# Capacidad: Máximo valor de flujo que puede circular por el arco.
   -# Valor de flujo: Valor actual del flujo que circula por el arco.
   .

   A una red de flujo también se le conoce como red capacitada. En una
   red capacitada se identifican nodos especiales denominados "fuentes"
   y "sumideros", respectivamente. Un nodo fuente es uno que no tiene
   arcos de entrada; consecuentemente, sólo emana flujo. Análogamente,
   un nodo sumidero es uno que no tiene arcos de salida; en el mismo
   sentido, sólo recibe flujo.

   Puesto que las abstracciones provienen de la teoría física, para cada
   nodo que no sea fuente o sumidero siempre se debe cumplir que la
   cantidad total de flujo de entrada debe ser la misma que la de salida.

   Las redes capacitadas son un excelente vehículo de resolución de
   problemas sobre grafos. Aparte del problema del flujo máximo,
   representable en muchas situaciones de la vida real, una red
   capacitada sirve para resolver otros problemas de optimización
   combinatoria: caminos mínimos, emparejamientos, cortes, etc.
   @}
   */
