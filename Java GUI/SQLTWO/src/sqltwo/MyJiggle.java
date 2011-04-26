package sqltwo;

// IMPORT COMMANDS

import jiggle.*;
import java.awt.*; import java.applet.Applet;
import java.io.*; import java.net.*; import java.util.*;
import java.awt.event.*;

// JIGGLEFRAME CLASS

public class MyJiggle extends Frame  {

	public static final long serialVersionUID = 1L;
	
	int frameWidth = 1024, frameHeight = 768;
	myMainForm parentApplet = null; int PORT;

        MyJiggleSettings settings = new MyJiggleSettings (this, frameWidth / 2, frameHeight / 2);;
        
	public Graph graph = null;
	public ForceModel forceModel = null;
	public FirstOrderOptimizationProcedure optimizationProcedure = null;
	public double vertexWidth = 10, vertexHeight = 10;
	public boolean useVertexSize = false;
        public float GData[][];
        public int GSize;
        public int optimizeTime = 100;

	boolean isIterating = false, iteratingNow = false;

	Image offScreenImage = null;
	Graphics offScreenGraphics = null;
	int imageWidth = 0, imageHeight = 0;

	MyJiggle (int nsize) {
		super ("Graphing using JIGGLE");
		setLayout (null);
		setBackground (Color.white);
		setFont (new Font ("TimesRoman", Font.BOLD, 16));
                GSize = nsize;
		setSize (frameWidth, frameHeight);
		setVisible (true);
		imageWidth = getSize ().width; imageHeight = getSize ().height;
		offScreenImage = createImage (imageWidth, imageHeight);
		offScreenGraphics = offScreenImage.getGraphics ();

	}


	public void setGraph (Graph g) {
		isIterating = false;
		while (iteratingNow)
			try {Thread.sleep (50);} catch (Exception e) {}
		graph = g;
	}

	void scramble () {
		int n = graph.numberOfVertices;
		double w = getSize ().width, h = getSize ().height;
		int d = graph.getDimensions ();
		for (int i = 0; i < n; i++) {
			double coords [] = graph.vertices [i].getCoords ();
			for (int j = 0; j < d; j++) coords [j] = Math.random () * w;
		}
		double sumX = 0, sumY = 0;
		for (int i = 0; i < n; i++) {
			double coords [] = graph.vertices [i].getCoords ();
			sumX += coords [0]; sumY += coords [1];
		}
		for (int i = 0; i < n; i++) {
			Vertex v = graph.vertices [i];
			double coords [] = graph.vertices [i].getCoords ();
			coords [0] += (w / 2) - (sumX / n);
			coords [1] += (h / 2) - (sumY / n);
		}
	}

        public void doScramble()
        {
            scramble();
            repaint();
        }


	public void update (Graphics g) {
		if (graph == null) return;
                Integer Left, Right;
                double distance;

		int width = getSize ().width, height = getSize ().height;
		if ((width != imageWidth) || (height != imageHeight)) {
			imageWidth = width; imageHeight = height;
			offScreenImage = createImage (width, height);
			offScreenGraphics = offScreenImage.getGraphics ();
		}
		offScreenGraphics.setColor (Color.white);
		offScreenGraphics.fillRect (0, 0, width, height);
		offScreenGraphics.setColor (Color.black);
                for (int i = 0; i < graph.numberOfEdges; i++) {
			Edge e = graph.edges [i];
			Vertex from = e.getFrom (), to = e.getTo ();
			double f [] = from.getCoords (), t [] = to.getCoords ();
			int x1 = (int) f [0], y1 = (int) f [1];
			int x2 = (int) t [0], y2 = (int) t [1];

                        Left = Integer.parseInt(from.getName());
                        Right = Integer.parseInt(to.getName());
                        distance = (double)GData[Left][Right];
                        if(distance == 0)
                            e.setPreferredLength(10D);
                        else
                            e.setPreferredLength(distance);

                        if((float) distance != 0F)
                        {
                            //System.out.println("Debug distance "+ Left + " " + Right + " D: " + (float) distance );
                            offScreenGraphics.drawLine (x1, y1, x2, y2);
                        }

		}
		for (int i = 0; i < graph.numberOfVertices; i++) {
			Vertex v = graph.vertices [i];
			double coords [] = v.getCoords ();
			int x = (int) coords [0], y = (int) coords [1];
			int w = (int) vertexWidth+14, h = (int) vertexHeight+10;
                        //x,y,width,height
                        offScreenGraphics.setColor (Color.white);
			offScreenGraphics.fillRect (x - w / 2, y - h / 2, w, h);
                        offScreenGraphics.setColor (Color.black);
			offScreenGraphics.drawRect (x - w / 2, y - h / 2, w, h);
                        offScreenGraphics.drawString(Integer.toString(i+1), x-w/2, y+5);
		}

		g.drawImage (offScreenImage, 0, 0, this);
	}

        public void updateData(int n, float data[][])
        {

            Integer i = 0;

            //Copy Data
            GData = new float[n][n];
            System.out.println("Length " +java.lang.reflect.Array.getLength(data));
            System.arraycopy(data, 0, GData, 0, java.lang.reflect.Array.getLength(data)-1);


            Graph g = null;
            g = new MyGraph(n, data);
            setGraph(g);
            scramble();
            update(offScreenGraphics);
            settings.applySettings();

            while(i<optimizeTime)
            {
                optimizationProcedure.improveGraph ();
              //repaint();
              i++;
            }
             repaint();
            System.out.println("getData-Finished");
        
        }

        public void doOptimize()
        {
            int i = 0;
            while(i<optimizeTime)
            {
                optimizationProcedure.improveGraph ();
              //repaint();
              i++;
            }
             repaint();
        }
        
        public void refresh(float data[][])
        {
            //Copy Data
            GData = new float[GSize][GSize];
            System.out.println("Length " +java.lang.reflect.Array.getLength(data));
            System.arraycopy(data, 0, GData, 0, java.lang.reflect.Array.getLength(data)-1);
            
            System.out.println("Refresh");
            update(offScreenGraphics);
            repaint();
        }

        public void testoptimizeGraph()
        {
           Integer i = 0;
           Integer n = 7;
           update(offScreenGraphics);
           settings.applySettings();

           while(i<n*10)
            {
              optimizationProcedure.improveGraph ();
              i++;
            }

           repaint();

        }



}
