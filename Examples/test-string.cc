
# include <iostream>
# include <ah-string-utils.H>

const string text =
  "Esta es la historia de un programador llamado Alejandro que era admirado por Leandro por su velocidad de programacion. Pero el gran problema de Alejandro era que hablaba mucho mas rapido que la velocidad de pensamiento de Leandro. Consecuentemente, Leandro nunca lograba entender a Alejandro";

int main()
{
  split_text_into_words(text).
    for_each([] (const auto & s) { cout << s << endl; });

  cout << justify_text(text, 50, 10) << endl
       << endl
       << align_text_to_left(text, 50, 4) << endl
       << endl
       << shift_lines_to_left(justify_text(text, 60), 2) << endl
       << "FIN!" << endl;

  string str = "Hipógrifo violento que corriste parejas con el viento";
  for (size_t i = 0; i < str.size() + 1; ++i)
    {
      auto p = split_pos(str, i);
      cout << "  " << p.first << " " << i << " " << p.second << endl;
    }

  auto l = split_n(str, 7);
  for (auto it = l.get_it(); it.has_curr(); it.next())
    cout << it.get_curr() << "|";
  cout << endl;

  string s= "camelCaseString";
  split_camel_case(s).for_each([] (const auto & s) { cout << s << endl; });

  return 0;
}
