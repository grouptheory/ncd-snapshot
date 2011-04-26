package sqltwo;

import jiggle.*;

public class MyGraph extends Graph {

	public MyGraph (int images, float data[][]) {super (); initialize (images, data);}

	private void initialize (int images, float data[][]) {
		Vertex V [] = new Vertex [images];

                
		for (int i = 0; i < images; i++)
                {
			V [i] = insertVertex ();
                        V[i].setName(Integer.toString(i));
                }

               // int i = 1;
		for (int i = 0; i < images; i++)
                   for(int j = 0; j < images; j++)
                    {
                     if(data[i][j] > 0)
                     {
                         insertEdge(V[i], V[j]);
                         System.out.println("insertEdge V "+i+" V "+j);
                     }
                    }


	}
}
