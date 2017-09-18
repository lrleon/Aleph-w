Node * random_tree(int n)
{
  if (n == 0) return NULL;

  Node * root = new Node; // apartar mem nodo (porque aún no existe)

  if (n == 1) return root;

      /* sorteo que define aleatoriamente la posición infija de la raíz */
  int r = /* aleatorio entre 1 y n */;

  L(root) = random_tree(r - 1); // aleatorio de r - 1 nodos
  R(root) = random_tree(n - r); // aleatorio de n - r nodos

  return root;
}

void enlazar(Node * r, Dlink * head)
{
  if (r == NULL) return;

  enlazar(L(r), head);

  heap.append(&r->link);

  enlazar(R(r), head);
}
