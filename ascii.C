# include <iostream>
# include <ahUtils.H>

struct Two_Nibbles
{
  unsigned int n1 : 4;
  unsigned int n2 : 4;

  Two_Nibbles() : n1(0), n2(0) {}

  Two_Nibbles & operator = (const char & c)
  {
    Two_Nibbles * ptr = (Two_Nibbles*) &c;

    n1 = ptr->n1;
    n2 = ptr->n2;

    return *this;
  }
};


char* stringficate(void *src_data, size_t src_size, char * result) 
{
  Two_Nibbles two_nibbles;

  char * ret_val = result;
  char * src_str = static_cast<char*> (src_data);

  for (int i = 0; i < src_size; ++i)
    {                
      two_nibbles = * src_str;
      src_str++;
      *result++ = Aleph::nibble_to_char(two_nibbles.n2);
      *result++ = Aleph::nibble_to_char(two_nibbles.n1);
    }

  *result = '\0';

  return ret_val;
}

int main()
{
  char result[256];
  for (int i = 0; i < 256; i++)
    {
      stringficate(&i, 1, result);
      std::cout << i << " = " << (char) i << " " << result << std::endl;
    }
}
