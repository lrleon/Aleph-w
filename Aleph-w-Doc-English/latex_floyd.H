
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file latx_floyd.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef FLOYD_H
# define FLOYD_H

# include <ahFunction.H>
# include <tpl_matgraph.H>
# include <mat_latex.H>

namespace Aleph
{


    template <class AM>
struct Initialize_Dist
{
  typedef typename AM::Graph_Type::Arc_Type Arc_Type;

  typedef typename AM::Graph_Type GT;

  typedef typename GT::Node Node;

  typedef typename GT::Arc Arc;

  typedef typename AM::Arc_Type::Distance_Type Distance_Type;

  void operator () (AM &            mat,
		    Node *          src,
		    Node *          tgt,
		    const long &    i,
		    const long &    j,
		    Distance_Type & entry,
		    void *          p)
  {
    Ady_Mat<typename AM::Graph_Type, long> & path = 
      * reinterpret_cast<Ady_Mat<typename AM::Graph_Type, long> *>(p);

    if (i == j)
      {
	entry = AM::Graph_Type::Arc_Type::Zero_Distance;
	path(i, j) = j;

	return;
      }

    GT & g = mat.get_list_graph();

    Arc * arc = search_arc(g, src, tgt);

    if (arc == NULL)
      {
	entry = AM::Graph_Type::Arc_Type::Max_Distance;
	return;
      }

    entry = arc->get_info().get_distance();

    path(i, j) = j;
  }
};


/**
  @note GT :: Arc_Type you must export the type GT :: :: Distance_Type and two Arc_Type
  Such constant calls:

  Max_Distance: Corresponding to infinity
  Zero_Distance: nul Reported
*/

    template <class GT, class Compare, class Plus> 
void floyd_all_shortest_paths
(GT &                                                g, 
 Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
 Ady_Mat<GT, long> &                                 path)
{
  typedef Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> Dist_Mat;

  typedef typename GT::Arc_Type::Distance_Type Dist_Type;

  dist.
    template operate_all_arcs_matrix<Initialize_Dist<Dist_Mat> > (&path);

  const Dist_Type & max = GT::Arc_Type::Max_Distance;

  const long & n = g.get_num_nodes();

  for (int i = 0; i < n; ++i)
    for (int s = 0; s < n; ++s)
      if (dist(s, i) < max)
	for (int t = 0; t < n; ++t)
	  {
	    Dist_Type new_dist = Plus () (dist(s, i), dist(i, t));

	    if (Compare () (new_dist, dist(s, t)))
	      {
		path(s, t) = path(s, i);
		dist(s, t) = new_dist;
	      }
	  }
}




    template <class GT> 
void floyd_all_shortest_paths
(GT &                                                g, 
 Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
 Ady_Mat<GT, long> &                                 path)
{
  typedef typename GT::Arc_Type::Distance_Type Dist_T;

  floyd_all_shortest_paths<GT, Aleph::less<Dist_T>, Aleph::plus<Dist_T> >
    (g, dist, path);
}


    template <class Mat>
void find_min_path(Mat &                                 p, 
		   const long &                          src_index,
		   const long &                          tgt_index,
		   Path<typename Mat::Graph_Type> & path)
{
  typedef typename Mat::Graph_Type GT;
  typedef typename GT::Node Node;

  GT & g = p.get_list_graph();

  Node * src = p(src_index);

  path.set_graph(g, src);
  
  for (long i = src_index, j; j != tgt_index; i = j)
    {
      j = p(i, tgt_index);

      Node * tgt = p(j);

      path.append(tgt);
    }
}

    template <class Mat>
void find_min_path(Mat &                                 p, 
		   typename Mat::Node *                  src_node,
		   typename Mat::Node *                  tgt_node,
		   Path<typename Mat::Graph_Type> & path)
{
  const long src_index = p(src_node);
  const long tgt_index = p(tgt_node);

  find_min_path(p, src_index, tgt_index, path);
}

static const size_t Buf_Size = 10;


  template <class GT, class Compare, class Plus, 
	    template <class> class P_i,  /** Index format fil / col dist and path */
	    template <class> class P_ij, /** Path format(i, j) */
	    template <class> class D_ij> /** dist format (i, j) */
void floyd_all_shortest_paths_latex
(GT &                                                g, 
 Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
 Ady_Mat<GT, long> &                                 path,
 ofstream &                                          output)
{
  typedef Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> Dist_Mat;
  typedef Ady_Mat<GT, long>                                 Path_Mat;

  typedef typename GT::Arc_Type::Distance_Type Dist_Type;

  dist.
    template operate_all_arcs_matrix<Initialize_Dist<Dist_Mat> > (&path);

  const Dist_Type & max = GT::Arc_Type::Max_Distance;

  const long & n = g.get_num_nodes();

  output << "\\begin{figure}[H]{\\tiny " << endl
	 << "\\begin{tabular}{ll}" << endl
	 << "\\begin{tabular}{ll}" << endl;
  mat_to_latex <Dist_Mat, P_i<Dist_Mat>, P_i<Dist_Mat>, D_ij<Dist_Mat> >
    (dist, n, n, output, "\\hskip -5mm $D_0=$", "\\\\ ");
  output << "\\end{tabular}" << endl
	 << " & \\begin{tabular}{ll}" << endl;
  mat_to_latex <Path_Mat, P_i<Path_Mat>, P_i<Path_Mat>, P_ij<Path_Mat> >
    (path, n, n, output, "\\hskip -7mm $P_0=$", "\\\\ ");
  output << "\\end{tabular}" << endl 
    	 << "\\end{tabular}" << endl
	 << "}\\end{figure}" << endl;

  for (int i = 0; i < n; ++i)
    {
      for (int s = 0; s < n; ++s)
	if (dist(s, i) < max)
	  for (int t = 0; t < n; ++t)
	    {
	      Dist_Type new_dist = Plus() (dist(s, i), dist(i, t));

	      if (Compare () (new_dist, dist(s, t)))
		{
		  path(s, t) = path(s, i);
		  dist(s, t) = new_dist;
		}
	    }
      char buf[256];

      snprintf(buf, 256, "\\hskip -5mm $D_%d=$ ", i + 1);

      output << "\\begin{figure}[H]{\\tiny " << endl
 	     << "\\begin{tabular}{ll}" << endl
 	     << "\\begin{tabular}{ll}" << endl;
      mat_to_latex <Dist_Mat, P_i<Dist_Mat>, P_i<Dist_Mat>, D_ij<Dist_Mat> >
	(dist, n, n, output, buf, "\\\\ ");
      output << "\\end{tabular}" << endl
	     << " & \\begin{tabular}{ll}" << endl;

      snprintf(buf, 256, "\\hskip -7mm $P_%d=$ ", i + 1);

      mat_to_latex <Path_Mat, P_i<Path_Mat>, P_i<Path_Mat>, P_ij<Path_Mat> >
	(path, n, n, output, buf, "\\\\");
      output << "\\end{tabular}" << endl 
	     << "\\end{tabular}" << endl
	     << "}\\end{figure}" << endl;
    }
}


template <class GT, 
	  template <class> class P_i,  /** Index format fil/col dist and path */
	  template <class> class P_ij, /** path format (i, j) */
	  template <class> class D_ij> /** dist format (i, j) */
void floyd_all_shortest_paths_latex
(GT &                                                g, 
 Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
 Ady_Mat<GT, long> &                                 path,
 ofstream &                                          output)
{
  typedef Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> Dist_Mat;
  typedef typename GT::Arc_Type::Distance_Type Dist_T;

  floyd_all_shortest_paths_latex
    <GT, Aleph::less<Dist_T>, Aleph::plus<Dist_T>, P_i, P_ij, D_ij> 
      (g, dist, path, output);
}



} /* End namespace Aleph */

# endif // FLOYD_H
