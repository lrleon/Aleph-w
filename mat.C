

# include <iostream>
# include <ahAssert.H>

class Mat
{
private:

  struct Nodo
  {
    int fil, col;
    int valor;
    Nodo *derecho, *abajo;
  };

  int n;
  
  Nodo** fila;
  Nodo** columna;

  void insert(Nodo*& predFil, Nodo*& predCol, int i, int j, int valor)
    {
      Nodo *nuevo      = new Nodo;
      nuevo->valor     = valor;
      nuevo->fil       = i;
      nuevo->col       = j;
      nuevo->derecho   = predFil->derecho;
      predFil->derecho = nuevo;
      predFil          = nuevo;
      nuevo->abajo     = predCol->abajo;
      predCol->abajo   = nuevo;
      predCol          = nuevo;
    }

  friend struct Proxy
  {
    Mat& mat;
    Nodo *predFil, *predCol;
    int  *valorPtr;
    int fila, columna;
      
    Proxy(Mat& m, int i, int j)
      : mat(m), valorPtr(NULL), fila(i), columna(j)
      {
	assert(j >= 0 && j < mat.n);

	// Busque posicion anterior al indice de la fila
	Nodo *cabFil = mat.fila[i], *p;
	predFil      = cabFil;
	p            = cabFil->derecho;
	while (p != cabFil)
	  {
	    if (p->col == j)
	      {
		valorPtr = &(p->valor); // Encontrado!
		return;
	      }
	    if (p->col > j) 
	      break;
	    predFil = p;
	    p       = p->derecho;
	  }
	// Busque posicion anterior al indice de la columna
	Nodo* cabCol = mat.columna[j];
	predCol      = cabCol;
	p            = cabCol->abajo;
	while (p != cabCol)
	  {
	    assert(p->fil != i);
	    if (p->fil > i)
	      break;
	    predCol = p;
	    p       = p->abajo;
	  }
	// Aqui predFil apunta al predecesor de la fila y predCol
	// apunta al predecesor de columna
      }

    Proxy& operator = (int valor)
      {
	if (valor == 0)
	  return *this;
	if (valorPtr != NULL)
	  {
	    *valorPtr = valor;
	    return *this;
	  }
	Nodo* nuevo = new Nodo;
	nuevo->valor     = valor;
	nuevo->fil       = fila;
	nuevo->col       = columna;
	nuevo->derecho   = predFil->derecho;
	nuevo->abajo     = predCol->abajo;
	predFil->derecho = nuevo;
	predCol->abajo   = nuevo;
	return *this;
      }

    operator int() const
      {
	if (valorPtr != NULL)
	  return *valorPtr;
	return 0;
	} 
  };

public:

  Mat(int dim)
    : n(dim)
    {
      int i;
      Nodo* nodo;
      fila    = new Nodo* [n];
      columna = new Nodo* [n];

      for (i = 0; i < n; i++) 
	{
	      // Iniciar fila
	  nodo          = new Nodo;
	  nodo->fil     = -1;
	  nodo->col     = -1;
	  nodo->derecho = nodo;
	  nodo->abajo   = nodo;
	  fila[i]       = nodo;
	      // iniciar columna
	  nodo          = new Nodo;
	  nodo->fil     = -1;
	  nodo->col     = -1;
	  nodo->derecho = nodo;
	  nodo->abajo   = nodo;
	  columna[i]    = nodo;
	}
    }

  ~Mat()
    {
      int i;
      Nodo *nodo, *cab, *aux;
      
      for (i = 0; i < n; i++) 
	{
	  cab  = fila[i];
	  nodo = cab->derecho;
	  while (nodo != cab)
	    {
	      aux  = nodo;
	      nodo = nodo->derecho;
	      delete aux;
	    }
	  assert(nodo == cab);
	  delete cab;
	  delete columna[i];
	}

      delete [] fila;
      delete [] columna;
    }

  Mat(const Mat& mat)
    : n(mat.n)
    {
      int i;
      Nodo* nodo;
      Nodo *actual, *cab, *aux;
      Nodo *predFil, **predCol;

      fila    = new Nodo* [n];
      columna = new Nodo* [n];

      predCol = new Nodo* [n];

      for (i = 0; i < n; i++) 
	{
	      // Iniciar fila
	  nodo          = new Nodo;
	  nodo->fil     = -1;
	  nodo->col     = -1;
	  nodo->derecho = nodo;
	  nodo->abajo   = nodo;
	  fila[i]       = nodo;
	      // iniciar columna
	  nodo          = new Nodo;
	  nodo->fil     = -1;
	  nodo->col     = -1;
	  nodo->derecho = nodo;
	  nodo->abajo   = nodo;
	  columna[i]    = predCol[i] = nodo;
	}

      for (i = 0; i < n; i++)
	{
	  actual  = mat.fila[i]->derecho;
	  predFil = fila[i];
	  while (actual->col >= 0)
	    {
	      insert(predFil, predCol[actual->col], i, actual->col,
		     actual->valor); 
	      actual = actual->derecho;
	    }
	}
      delete [] predCol;
    }

  Mat& operator = (const Mat& mat)
    {
      assert(n == mat.n);

      Nodo *actual, *cab, *aux;
      Nodo *predFil, **predCol;
      int i;

      predCol = new Nodo* [n];

      for (i = 0; i < n; i++) 
	{
	  columna[i]->abajo = columna[i];
	  predCol[i]        = columna[i];
	  cab  = fila[i];
	  actual = cab->derecho;
	  while (actual != cab)
	    {
	      aux    = actual;
	      actual = actual->derecho;
	      delete aux;
	    }
	  cab->derecho = cab;
	}

      for (i = 0; i < n; i++)
	{
	  actual  = mat.fila[i]->derecho;
	  predFil = fila[i];
	  while (actual->col >= 0)
	    {
	      insert(predFil, predCol[actual->col], i, actual->col,
		     actual->valor); 
	      actual = actual->derecho;
	    }
	}
      delete [] predCol;
      return *this;
    }

  friend class Fila
  {
    friend class Mat;
    Mat&  mat;
    int   indexFil;

    Fila(Mat& m, int fila)
      : mat(m), indexFil(fila)
      {}

  public:

    const Proxy operator [] (int index) const
      {
	assert(index >= 0 && index < mat.n);

	return Proxy(mat, indexFil, index);
      }

    Proxy operator [] (int index) 
      {
	assert(index >= 0 && index < mat.n);

	return Proxy(mat, indexFil, index);
      }
  };

  const Fila operator [] (int index) const
    {
      assert(index >= 0 && index < n);
      return Fila(*this, index);
    }

  Fila operator [] (int index) 
    {
      assert(index >= 0 && index < n);
      return Fila(*this, index);
    }

 
  const Mat operator + (const Mat& der) 
    {
      assert(n == der.n);
      int i, j;
      int valor;
      Mat retVal(n);
      Nodo *predFilRV, **predColRV, *actIzq, *actDer;

      predColRV = new Nodo* [n];

      for (i = 0; i < n; i++)
	predColRV[i] = retVal.columna[i];

      for (i = 0; i < n; i++)
	{
	  actIzq    = fila[i]->derecho;
	  actDer    = der.fila[i]->derecho;
	  predFilRV = retVal.fila[i];
	  
	  while (1)
	    {
	      if (actIzq->col < 0 || actDer->col < 0)
		break;
	      if (actIzq->col == actDer->col)
		{
		  j      = actIzq->col;
		  valor  = actIzq->valor + actDer->valor;
		  actIzq = actIzq->derecho;
		  actDer = actDer->derecho;
		  if (valor == 0)
		    continue;
		}
	      else if (actIzq->col < actDer->col)
		{
		  j      = actIzq->col;
		  valor  = actIzq->valor;
		  actIzq = actIzq->derecho;
		}
	      else
		{
		  j     = actDer->col;
		  valor = actDer->valor;
		  actDer = actDer->derecho;
		}
	      insert(predFilRV, predColRV[j], i, j, valor);
	    }
	  while (actDer->col > 0)
	    {
	      insert(predFilRV,  predColRV[actDer->col], i, actDer->col,
		     actDer->valor);
	      actDer = actDer->derecho;
	    }
	  while (actIzq->col > 0)
	    {
	      insert(predFilRV,  predColRV[actIzq->col], i, actIzq->col,
		     actIzq->valor);
	      actIzq = actIzq->derecho;
	    }
	}
      delete [] predColRV;
      return retVal;
    }

  const Mat operator * (const Mat& der) 
    {
      assert(n == der.n);
      int i, j;
      int valor;
      Mat retVal(n);
      Nodo *predFilRV, **predColRV, *actFil, *actCol;

      predColRV = new Nodo* [n];

      for (i = 0; i < n; i++)
	predColRV[i] = retVal.columna[i];

      for (i = 0; i < n; i++)
	{
	  actFil    = fila[i]->derecho;
	  predFilRV = retVal.fila[i];

	  for (j = 0; j < n; j++)
	    {
	      actCol = der.columna[i]->abajo;
	      valor  = 0;
	  
	      while (1)
		{
		  if (actFil->col < 0 || actCol->fil < 0)
		    break;
		  if (actFil->col == actCol->fil)
		    {
		      valor  += actFil->valor * actCol->valor;
		      actFil = actFil->derecho;
		      actCol = actCol->abajo;
		    }
		  else if (actFil->col < actCol->fil)
		    actFil = actFil->derecho;
		  else
		    actCol = actCol->abajo;
		}

	      if (valor != 0)
		insert(predFilRV, predColRV[j], i, j, valor);
	    }
	}
      delete [] predColRV;
      return retVal;
    }

};



int main()
{
  Mat m1(4), m2(4), m3(4), m4(4);
  int i, j, val;

  m1[1][1] = 5;
  m1[3][0] = 2;

  m2[1][1] = 5;
  m2[3][0] = 2;

  m4 = m1 + m2;

  m3 = m1 * m2;

  for (i = 0; i < 4; i ++)
    {
      for (j = 0; j < 4; j++)
	{
	  val = m3[i][j];
	  cout << val << " ";
	}
      cout << "\n";
    }
}
