
# include <cstring>
# include <iostream>

using namespace std;

    template <typename T>
void reverse_array(T a[], int l, int r)
{
  for (int i = l, j = r; i < j; ++i,--j)
    std::swap(a[i], a[j]);
}

template <typename T>
void rotate(T a[], int l, int r, int n)
{
  reverse_array(a, l, l + n - 1);
  reverse_array(a, l + n, r);
  reverse_array(a, l, r);
}

template <typename T>
size_t count_words(T a[], const T & blank, int l, int r)
{
  size_t count = 0;

  for (int i = l; i <= r; )
    {
      while (a[i] == blank) 
	i++;
      
      while (a[i] != blank and a[i] != '\0') 
	i++;

      if (a[i] != '\0') 
	count++;
    }

  return count;
}

char a1[] = "abcde12345fghijklmnopqrstvwxyz";

char a2[81] = "  Esta   es una prueba de palabras  la";

int main()
{
  cout << a1 << endl;

  reverse_array(a1, 5, 9);

  cout << a1 << endl;

  rotate(a1, 5, 9, 3);

  cout << a1 << endl;

  cout << a2 << " tiene " << count_words(a2, ' ', 5, 5& 28) << endl;;

}
