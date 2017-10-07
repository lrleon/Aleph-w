
# include <cstring>
# include <iostream>
# include <fstream>

# include <Huffman.H>
# include <huffman_btreepic.H>


using namespace std;


char  poema_2 [] = 
"             Las cosas\n"
"\n"
"El bastón, las monedas, el llavero,\n"
"la dócil cerradura, las tardías\n"
"notas que no leerán los pocos días\n" 
"que me quedan, los naipes y el tablero,\n"
"\n"
"un libro y en sus páginas la ajada\n"
"violeta, monumento de una tarde\n"
"sin duda inolvidable y ya olvidada,\n"
"el rojo espejo occidental en que arde\n"
"\n"
"una ilusoria aurora. ¡Cuántas cosas,\n"
"láminas, umbrales, atlas, copas, clavos,\n"
"nos sirven como tácitos esclavos,\n"
"\n"
"ciegas y extrañamente sigilosas!\n"
"Durarán más allá de nuestro olvido;\n" 
"no sabrán nunca que nos hemos ido.\n"
"\n"
"            Jorge Luis Borges\n";

char  poema_1 [] = 
"Los Nacimientos\n"		
"\n"
"Nunca recordaremos haber muerto.\n"
"\n"
"Tanta paciencia\n"
"para ser tuvimos\n"
"anotando\n"
"los números, los días,\n"
"los años y los meses,\n"
"los cabellos, las bocas que besamos,\n"
"y aquel minuto de morir\n"
"lo dejaremos sin anotación:\n"
"se lo damos a otros de recuerdo\n"
"o simplemente al agua,\n"
"al agua, al aire, al tiempo.\n"
"Ni de nacer tampoco\n"
"guardamos la memoria,\n"
"aunque importante y fresco fue ir naciendo;\n"
"y ahora no recuerdas un detalle,\n"
"no has guardado ni un ramo\n"
"de la primera luz.\n"
"\n"
"Se sabe que nacemos.\n"
"\n"
"Se sabe que en la sala\n" 
"o en el bosque\n"
"o en el tugurio del barrio pesquero\n"
"o en los cañaverales crepitantes\n"
"hay un silencio enteramente extraño,\n"
"un minuto solemne de madera\n"
"y una mujer se dispone a parir.\n"
"\n"
"Se sabe que nacimos.\n"
"\n"
"Pero de la profunda sacudida\n"
"de no ser a existir, a tener manos,\n"
"a ver, a tener ojos,\n"
"a comer y llorar y derramarse\n"
"y amar y amar y sufrir y sufrir,\n"
"de aquella transición o escalofrío\n"
"del contenido eléctrico que asume\n"
"un cuerpo más como una copa viva,\n"
"y de aquella mujer deshabitada,\n"
"la madre que allí queda con su sangre\n"
"y su desgarradora plenitud\n"
"y su fin y comienzo, y el desorden\n"
"que turba el pulso, el suelo, las frazadas,\n"
"hasta que todo se recoge y suma\n"
"un nudo más el hilo de la vida,\n"
"nada, no quedó nada en tu memoria\n"
"del mar bravío que elevó una ola\n"
"y derribó del árbol una manzana oscura.\n"
"\n"
"No tienes más recuerdo que tu vida.\n"
"\n"
"					Pablo Neruda.\n";


char  poema_3 [] = 
"Muerte De Antoñito El Camborio\n"
"    Federico García Lorca\n"
"\n"
"   Voces de muerte sonaron\n"
"cerca del Guadalquivir.\n"
"Voces antiguas que cercan\n"
"voz de clavel varonil.\n"
"Les clavó sobre las botas\n"
"mordiscos de jabalí.\n"
"En la lucha daba saltos\n"
"jabonados de delfín.\n"
"Bañó con sangre enemiga\n"
"su corbata carmesí,\n"
"pero eran cuatro puñales\n"
"y tuvo que sucumbir.\n"  
"Cuando las estrellas clavan\n"
"rejones al agua gris,\n"
"cuando los erales sueñan\n"
"verónicas de alhelí,\n"
"voces de muerte sonaron\n"
"cerca del Guadalquivir.\n"
"\n"
"   Antonio Torres Heredia,\n"
"Camborio de dura crin,\n"
"moreno de verde luna,\n"
"voz de clavel varonil:\n"
"¿quién te ha quitado la vida\n"
"cerca del Guadalquivir?\n"
"Mis cuatro primos Heredias\n"
"hijos de Benamejí.\n"
"Lo que en otros no envidiaban,\n"
"ya lo envidiaban en mí.\n"
"Zapatos color corinto,\n"
"medallones de marfil,\n"
"y este cutis amasado\n"
"con aceituna y jazmín.\n"
"¡Ay Antoñito el Camborio,\n"
"digno de una Emperatriz!\n"
"Acuérdate de la Virgen\n"
"porque te vas a morir.\n"
"¡Ay Federico García,\n"
"llama a la Guardia Civil!\n"
"Ya mi talle se ha quebrado\n"
"como caña de maíz.\n"
"\n"
"   Tres golpes de sangre tuvo\n"
"y se murió de perfil.\n"
"Viva moneda que nunca\n"
"se volverá a repetir.\n"
"Un ángel marchoso pone\n"
"su cabeza en un cojín.\n"
"Otros de rubor cansado,\n"
"encendieron un candil.\n"
"Y cuando los cuatro primos\n"
"llegan a Benamejí,\n"
"voces de muerte cesaron\n"
"cerca del Guadalquivir.\n"
  ;


size_t read_and_encode(char *                   str, 
		       Huffman_Encoder_Engine & huffman_engine,
		       BitArray &               bit_stream)
{
  huffman_engine.read_input(str, true);

  const size_t bit_stream_len = huffman_engine.encode(str, bit_stream);

  return bit_stream_len;
}

static int count = 0;

struct Lee
{
  bool operator() (BinNode<string> * p, const char * str) const
  {
    //    if (str == NULL)
    //      return false;

    cout << "** " << ++::count << " (" << p << " " << LLINK(p) << " " 
	 << RLINK(p) << ")" << endl;
    //    if (LLINK(p) == NULL and RLINK(p) == NULL)
      {
	p->get_key() = str;
	cout << "    put " << "\"" << str << "\"" << endl;
	return true;
      }

    return false;
  }
};

//# include "lorca.tree"

int main(int argn, char *argc[])
{
  ofstream huf_output("huffman.Tree", ios::out);
  output_ptr = &huf_output;

  Huffman_Encoder_Engine encoder;

  size_t code_len = 0;

  BitArray code(0);

  if (argn == 1)
    {
      code_len = read_and_encode(poema_3, encoder, code);

      huffman_to_btreepic(encoder.get_freq_root());

      {
	ofstream output("lorca.tree");
	encoder.save_tree_in_array_of_chars("lorca", output);
      }      

//       {
// 	ofstream output("h.tree");
// 	encoder.save_tree(output);
//       }     

//       {
// 	ifstream input("h.tree");
// 	Huffman_Encoder_Engine e;
// 	e.load_tree(input);
// 	assert(areSimilar(encoder.get_root(), e.get_root()));
//       }

//       {
// 	BinNode<string> * root = 
// 	  load_tree_from_array<BinNode<string>, Lee>(lorca_cdp, 215, lorca_k);

// 	assert(areSimilar(encoder.get_root(), root));

// 	destroyRec(root);


//       cout << "|r1| = " << compute_cardinality_rec(encoder.get_root()) << endl
// 	   << "|r2| = " << compute_cardinality_rec(root) << endl;

//       }
      
      Huffman_Decoder_Engine decoder(encoder.get_root(), "");
      //      Huffman_Decoder_Engine decoder(root, "");

      decoder.decode(code, std::cout);

      std::cout << std::endl;

      destroyRec(encoder.get_root());

      destroyRec(encoder.get_freq_root());

      ofstream verso("huffman-verso.tex", ios::out);
      verso << "\\begin{verse}" << std::endl;
  
      char * curr = poema_3;

      while (*curr != '\0')
	{
	  if (*curr == '\n')
	    {
	      if (curr[1] == '\n')
		{
		  verso << " \\\\" << endl
			<< "\\ \\" << endl
			<< endl;
		  curr++;
		}
	      else if (curr[1] == '\0')
		;
	      else
		verso << " \\\\" << endl;
	    }
	  else 
	    verso << *curr; 

	  curr++;
	}
      verso << "\\end{verse}" << std::endl;
  
      ofstream stat1("huffman-stat1.tex", ios::out);
      stat1 << "$" << strlen(poema_3)*8 << "$";

      ofstream stat2("huffman-stat2.tex", ios::out);
      stat2 << "$" << code_len << "$";

      return 0;
    }


  ifstream input1(argc[1], ios::in);

  encoder.read_input(input1, true);
     
  huffman_to_btreepic(encoder.get_freq_root(), true);


  ifstream input2(argc[1], ios::in);

  BitArray bit_stream(1024*1024*500);

  encoder.encode(input2, bit_stream);



}
