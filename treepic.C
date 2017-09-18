# include <iostream>
# include <fstream>
# include <string.h>
# include <stdlib.h>
# include <search.h>
# include <math.h>

class data //struct used for printing the tree.
{
public:
  int inorder, he, UDI, LDI; 
  data() {inorder=0;he=0;UDI=0;LDI=0;}
  virtual ~data() {/*EMPTY*/};
};

class daux //only helps to build the data array.
{
public:
  int preorder, UDP, LDP; 
  daux() {preorder=0;UDP=0;LDP=0;}
  virtual ~daux() {/*EMPTY*/};
};

data *dat;
daux *aux;
ofstream output;
ifstream input;

char *help[] = {
  /*s*/"Silent output",						//Instructions
  /*a*/"Add TeX header",
  /*k*/"Keyless printing (Don't add Lower data, only Upper Data",
  /*b*/"Binary search tree preordered list",	//Lists Identificators
  /*i*/"Binary tree inordered key list",
  /*p*/"Binary tree preordered key list",
  /*u*/"Upper Data Inordered list",
  /*U*/"Upper Data Preordered list",
  /*d*/"Lower Data Inordered list",
  /*D*/"Lower Data Preordered list",
  /*f*/"Use this file                 <FILE>",//Options
  /*x*/"Resolution in pixels per inch (>0)",
  /*h*/"Height in pixels              (>0)",
  /*w*/"Width in pixels               (>0)",
  /*r*/"Radius of the nodes in pixels (>0)",
  /*help*/"Show this help"
};

char *tags[] = {//index variables:
  "s",	//0  2 <-flag (prime numbers)
  "a",	//1  3
  "k",	//2	 5
  "b",	//3	 7
  "i", 	//4	 11
  "p",	//5  13
  "u",	//6  17
  "U",	//7  19
  "d", 	//8  23
  "D",	//9  29
  "f", 	//10 file <- char*
  "x",	//11 res <- int
  "h",	//12 he
  "w",	//13 wi
  "r", 	//14 rad
  "help"     //15help
};

int indexes[] = {2,3,5,7,11,13,17,19,23,29};
int f[]={0,0,0,0,0,0,0,0}, *sh, fileSignal=0;
int flag=1; //in the worst case: 1*(2*3*5*11*13*19*29)=2363790;

char *file, *TeX;
int res=2400, he=1000, wi=500, rad=350; //initialization of the tree scales.
int de = 0, le=1000; //Default dimensions of the array.

void showH()  //when there are no arguments for the program, it prints this help.
{
  int i=0;
  cout<<"\ntreepic: LaTeX tree generator using eepic macros\n"
      <<"Version: 1.1b  Universidad de Los Andes, by:\n"
      <<"\nLeonardo Zuniga S.   <leoz@sistemas.ing.ula.ve>,<leoz@ing.ula.ve>\n"
      <<"Bladimir Contreras   <jbc@sistemas.ing.ula.ve>\n"
      <<"Carlos Acosta        <albania@sistemas.ing.ula.ve>\n"
      <<"\nFor: Leandro R. Leon <lrleon@cemisid.ing.ula.ve>\n"
      <<"\nUsage: treepic {INSTRUCTIONS} {[OPTION][VALUE]...} {[ID][LIST]...}\n"
      <<"\nInstructions Available:\n";
  for (i=0; i<3; i++) cout<<"   -"<<tags[i]<<"   "<<help[i]<<"\n";
  cout<<"\nOptions Available:\n";
  for (i=10; i<15; i++) cout<<"   -"<<tags[i]<<"   "<<help[i]<<"\n";
  cout<<"\nId's Available:\n";
  for (i=3; i<10; i++) cout<<"   -"<<tags[i]<<"   "<<help[i]<<"\n";
  cout<<"\nLower case with Upper case or (p&i) with b tags are incompatible\n";
  cout<<"See an example and permanent configuration in .treepic\a\n";
}

void setDepth(data *Dat, daux *Aux, int pl, int pr, int il, int ir, int level)
{
  int j=0;
  if (pl>pr) return;
  int pre=Aux[pl].preorder;
  for (j=il; j<ir; j++)
    {
      if (Dat[j].inorder==pre) break;
    }
  Dat[j].he=level;
  if (flag%19==0) Dat[j].UDI=Aux[pl].UDP;
  if (flag%29==0) Dat[j].LDI=Aux[pl].LDP;
  setDepth(Dat,Aux,pl+1,pl+j-il,il,j-1,level+1);
  setDepth(Dat,Aux,pl+1+j-il,pr,j+1,ir,level+1);
}

void getDepth(data *Dat, int Le)//this just sets the global variable de(pth).
{
  int j=0;
  for (j=0; j<Le; j++)
    de=(Dat[j].he>de)?Dat[j].he:de;
}

int compare( const void *arg1, const void *arg2 )
{
  return ((*(int**)arg1)<(*(int**)arg2))?-1:1;
}

void seeFile(int order)
{
  unsigned int g = 0, j = 0, k = 0, cont = 0;
  char *buffer;
  buffer = new char[65];
  //	ifstream input(file, ios::nocreate);
  ifstream input(file);
  if (!input)
    {
      if (order==0) 
	showH();
      return;
    }
  while (!input.eof())
    {
      input>>buffer;
      if (!strncmp(buffer,"-",1))
	{
	  for (j=0; j<3; j++)
	    {
	      for (k=1; k<strlen(buffer); k++)

		{
		  if (!strncmp((buffer+k),tags[j],1))
		    {
		      if (order==0) 
			flag *= indexes[j]; 
		      cont=1;
		    }
		}
	    }
	  if (cont==1)
	    {
	      cont=0;
	      continue;
	    }
	  for(j=3; j<10; j++)
	    {
	      if (!strcmp(buffer+1,tags[j]))
		{
		  if (order==0) flag*=indexes[j];
		  cont=3;
		  break;
		}
	    }
	  if (cont==3)
	    {
	      cont=2;
	      g=0;
	      continue;
	    }
	  for(j=10; j<15; j++)
	    {
	      if (!strcmp(buffer+1,tags[j]))
		{
		  input>>buffer;
		  if (order==0)
		    {
		      switch (j)
			{
			case 10:
			  showH();
			  return;
			case 11:
			  res=atoi(buffer);
			  break;
			case 12:
			  he=atoi(buffer);
			  break;
			case 13:
			  wi=atoi(buffer);
			  break;
			case 14:
			  rad=atoi(buffer);
			  break;
			}
		      break;
		    }
		}
	      else 
		{
		  showH();
		  return;
		}
	    }
	}
      if (cont==2)
	{
	  if (order==0)
	    {
	      f[j-3]++;
	      f[0]+=(f[0]<f[j-3])?1:0;
	    }
	  else if (order==1)
	    {
	      switch (j)
		{
		case 3:
		  sh[g]=atoi(buffer);
		  aux[g].preorder=atoi(buffer);
		  break;
		case 4:
		  dat[g].inorder=atoi(buffer);
		  break;
		case 5:
		  aux[g].preorder=atoi(buffer);
		  break;
		case 6:
		  dat[g].UDI=atoi(buffer);
		  break;
		case 7:
		  aux[g].UDP=atoi(buffer);
		  break;
		case 8:
		  dat[g].LDI=atoi(buffer);
		  break;
		case 9:
		  aux[g].LDP=atoi(buffer);
		  break;
		}
	      g++;
	    }
	}
    }
  delete buffer;
  return;
}

int setArrays(unsigned int arc, char *arv[], int order) //this is a preparser of the data, checking its integrity.
{														//and returning the length of the arrays
  unsigned int fi=0, g=0, i=1, j=0, k=0, cont=0;
  if (arc<2)
    {
      if (order==0 && fileSignal==0) file="FILE";
      seeFile(order);
      fi=1;
    }
  else for (i=1; i<arc; i++)
    {
      if (!strncmp(arv[i],"-",1))
	{
	  if (!strcmp((arv[i]+1),tags[15]))
	    {
	      showH();
	      return (-1);
	    }
	  for (j=0; j<3; j++)
	    {
	      for (k=1; k<strlen(arv[i]); k++)
		{
		  if (!strncmp((arv[i]+k),tags[j],1))
		    {
		      if (order==0) flag*=indexes[j]; //detects joined letters: -sak
		      cont=1;
		    }
		}
	    }
	  if (cont==1)
	    {
	      cont=0;
	      continue;
	    }
	  for(j=3; j<10; j++)
	    {
	      if (!strcmp((arv[i]+1),tags[j]))
		{
		  if (order==0) flag*=indexes[j];
		  cont=3;
		  break;
		}
	    }
	  if (cont==3)
	    {
	      cont=2;
	      g=0;
	      continue;
	    }
	  for(j=10; j<15; j++)
	    {
	      if (!strcmp((arv[i]+1),tags[j]))
		{
		  i++;
		  if (order==0)
		    {
		      switch (j)
			{
			case 10:
			  file=arv[i];
			  seeFile(order);
			  fi=1;
			  break;
			case 11:
			  res=atoi(arv[i]);
			  break;
			case 12:
			  he=atoi(arv[i]);
			  break;
			case 13:
			  wi=atoi(arv[i]);
			  break;
			case 14:
			  rad=atoi(arv[i]);
			  break;
			}
		      break;
		    }
		}
	    }
	}
      if (cont==2)
	{
	  if (order==0)
	    {
	      f[j-3]++;
	      f[0]+=(f[0]>f[j-3])?1:0;
	    }
	  else if (order==1)
	    {
	      switch (j)
		{
		case 3:
		  sh[g]=atoi(arv[i]);
		  aux[g].preorder=atoi(arv[i]);
		  break;
		case 4:
		  dat[g].inorder=atoi(arv[i]);
		  break;
		case 5:
		  aux[g].preorder=atoi(arv[i]);
		  break;
		case 6:
		  dat[g].UDI=atoi(arv[i]);
		  break;
		case 7:
		  aux[g].UDP=atoi(arv[i]);
		  break;
		case 8:
		  dat[g].LDI=atoi(arv[i]);
		  break;
		case 9:
		  aux[g].LDP=atoi(arv[i]);
		  break;
		}
	      g++;
	    }
	}
    }
  if (fi==0) seeFile(order);
  if (order==0)
    {
      for (j=1; j<7; j++)
	{
	  if(f[j]!=0 && f[j]!=f[0])
	    {
	      showH();
	      return (-1);
	    }
	}
      return f[0];
    }
  return 0;
}

void loadConf()
{
  //	ifstream config(".treepic", ios::nocreate);
  ifstream config(".treepic");
  if(!config)
    {
      cout<<"no config file, using program defaults...";
      return;
    }
  config>>rad;
  config>>wi;
  config>>he;
  config>>res;
  config>>file;
  fileSignal=1;
  config.close();
  return;
}

void setConf()
{
  ofstream confi(".treepic", ios::trunc);
  confi<<rad<<"\t"<<wi<<"\t"<<he<<"\t"<<res<<"\t"<<file;
  confi<<"\nrad\twi\the\tres\tfile\n";
  confi<<"Example:\n";
  confi<<"treepic -sa -f FILE.txt\n";
  confi<<"treepic -a\n-i 1  3  4  5  6  8  10 11 12 13 15 17 19 20 21 24 25 27 28 30 31 33 34\n-p 19 10 4  1  3  6  5  8  13 11 12 15 17 27 21 20 25 24 33 30 28 31 34\n-U 12 6  2  0  1  4  3  5  9  7  8  10 11 17 14 13 16 15 21 19 18 20 22\n-D 3  4  2  4  3  4  1  3  4  2  3  4  0  3  2  4  3  1  4  3  4  2  3";
  confi.close();
  return;
}

void print(int Le, int De)
{
  if (flag%2!=0) cout<<"writting to file.\r";
  if (flag%3!=0)
    {
      output<<"%\n% Languaje in use is EEpic\n%\n";
    }
  if (flag%3==0)
    {
      output << "\\documentclass[11pt]{article}\n"
	     << "\\usepackage{eepic}\n"
	     << "\\begin{document}\n"
	     << "\\begin{center}\n";
    }
   output<<"\\setlength{\\unitlength}{"<<(1.0/(double)res)<<"in}\n%\n";
   output<<"\\begingroup\\makeatletter\\ifx\\SetFigFont\\undefined%\n";
   output<<"\\gdef\\SetFigFont#1#2#3#4#5{%\n";
   output<<"  \\reset@font\\fontsize{#1}{#2pt}%\n";
   output<<"  \\fontfamily{#3}\\fontseries{#4}\\fontshape{#5}%\n";
   output<<"  \\selectfont}%\n";
  output<<"\\fi\\endgroup%\n";
  //  output<<"{\\renewcommand{\\dashlinestretch}{30}\n";
  output<<"\\begin{picture}("<<(wi*(Le+1))<<","<<(he*(De+1))<<")(0,-10)"<<endl;
}

void print(data *Dat, int start, int Le, int level, int De)
{
  int f=0, i, j; 
  double x, y, m;
  for (j=start; j<start+Le; j++)
    {
      if (Dat[j].he==level) break;
    }
  int cX=(wi*(j+1));
  int cY=((he/2)+(he*(De-Dat[j].he)));
  if (flag%2!=0) output<<"%%%%%%%%%%%%%%%Node "<<Dat[j].inorder<<":\n";
  output<<"\\put("<<cX<<","<<cY<<"){\\ellipse{"<<rad*2<<"}{"<<rad*2<<"}}"<<endl;
  if (flag%17==0 || flag%19==0) output<<"\\put("<<cX-80<<","<<cY+rad+35<<"){\\makebox(0,0)[lb]{\\smash{{{\\SetFigFont{8}{9.6}{\\rm"  <<  "default}{\\mddefault}{\\updefault}"<<Dat[j].UDI<<"}}}}}\n";
  if (flag%5!=0) 
    {
      if (flag%23==0 || flag%29==0)
	{
	  output<<"\\put("<<cX-104<<","<<cY+44<<"){\\makebox(0,0)[lb]{\\smash{{{\\SetFigFont{8}{9.6}{\\family"<<"default}{\\mddefault}{\\updefault}"<<Dat[j].inorder<<"}}}}}\n";
	  output<<"\\path("<<cX-rad<<","<<cY<<")("<<cX+rad<<","<<cY<<")"<<endl;
	  output<<"\\put("<<cX-85<<","<<cY-255<<"){\\makebox(0,0)[lb]{\\smash{{{\\SetFigFont{8}{9.6}{\\family"<<"default}{\\mddefault}{\\updefault}"<<Dat[j].LDI<<"}}}}}\n";
	}
      else output<<"\\put("<<cX-104<<","<<cY-20<<"){\\makebox(0,0)[lb]{\\smash{{{\\SetFigFont{8}{9.6}{\\family"<<"default}{\\mddefault}{\\updefault}"<<Dat[j].inorder<<"}}}}}\n";
    }
  for (i=start; i<j; i++)
    {
      if (Dat[i].he==level+1)
	{
	  f+=1;
	  m=(double)he/(double)((j-i)*wi);
	  x=(double)rad*sqrt((1.0/((m*m)+1.0)));
	  y=m*x;
	  output<<"\\path("<<(wi*(i+1)+(int)x)<<","<<(he*(De-level)+(int)(y)-(0.5*he))<<")("<<(wi*(j+1)-(int)x)<<","<<(he*(De-level)-(int)(y)+(0.5*he))<<")\n";
	  break;
	}
    }
  for (i=j; i<start+Le; i++)
    {
      if (Dat[i].he==level+1)
	{
	  f+=2;
	  m=(double)he/(double)((j-i)*wi);
	  x=(double)rad*sqrt((1.0/((m*m)+1.0)));
	  y=m*x;
	  output<<"\\path("<<(wi*(i+1)-(int)x)<<","<<(he*(De-level)-(int)(y)-(0.5*he))<<")("<<(wi*(j+1)+(int)x)<<","<<(he*(De-level)+(int)(y)+(0.5*he))<<")\n";
	  break;
	}
    }
  if (f==1||f==3) print(Dat,start,j-start,level+1,De);
  if (f==2||f==3) print(Dat,j,start+Le-j,level+1,De);
  return;
}

void print()
{
  if (flag%2!=0) cout<<"writting to file.\r";
  output<<"\\end{picture}\n";
  if (flag%3==0)output<<"\\end{center}\n\\end{document}\n";
}

int main(unsigned int argn, char *argc[])
{
  file = new char[65];// = "FILE.any";
  TeX = new char[65]; // = "FILE.tex";
  loadConf();
  le=setArrays(argn, argc,0);
  setConf();
  if (le<1)
    {
      return (-1);
    }
  if (flag%7==0) sh = new int[le];
  dat = new data[le];
  aux = new daux[le];
  f[0]=setArrays(argn,argc,1);
  if (flag%7==0)
    {
      qsort((void *)sh,(size_t)le,sizeof(int *),compare);
      for (int q=0; q<le; q++) dat[q].inorder=sh[q];
    }
  setDepth(dat, aux, 0, le-1, 0, le-1, 0);
  getDepth(dat, le);
  TeX=strncpy(TeX,file,(strlen(file)-strlen(strrchr(file,'.'))));
  TeX[strlen(file)-strlen(strrchr(file,'.'))]='\0';
  TeX=strcat(TeX,".eepic");
  output.open(TeX,ios::out);
  print(le, de);
  print(dat, 0, le, 0, de);
  print();
  delete(TeX);
  //delete(aux);
  //delete(dat);	
  return (0);
}

