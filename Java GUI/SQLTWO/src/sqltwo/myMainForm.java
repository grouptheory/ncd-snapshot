/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * myMainForm.java
 *
 * Created on Apr 5, 2011, 7:15:16 PM
 */

package sqltwo;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import javax.imageio.ImageIO;
import java.net.URL;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.net.URL;
import javax.imageio.ImageIO;
import javax.swing.*;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.axis.CategoryAxis;
import org.jfree.chart.axis.CategoryLabelPositions;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.plot.CategoryPlot;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.renderer.category.BarRenderer;
import org.jfree.data.category.CategoryDataset;
import org.jfree.data.category.DefaultCategoryDataset;
import org.jfree.ui.ApplicationFrame;
import org.jfree.ui.RefineryUtilities;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.xy.XYDataset;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;

/**
 *
 * @author Richard
 */
public class myMainForm extends javax.swing.JFrame {

    /** Creates new form myMainForm */
    public myMainForm() {
        initComponents();
    }
    public static Connection doConnectsql(String connURL, String user, String pass)
    {
        Connection conn = null;
        try {
            conn = DriverManager.getConnection(connURL, user, pass);

        }//try Connection
        catch (SQLException ex) {
            // handle any errors
            System.out.println("SQLException: " + ex.getMessage());
            System.out.println("SQLState: " + ex.getSQLState());
            System.out.println("VendorError: " + ex.getErrorCode());
        }//Catch Connection

        return conn;
    }

    public static void SetupTables(Connection conn, int querynum, float cutoff, int imagetotal)
    {
        String SQLBuffer;

        Statement stmt = null;

        try {
            stmt = conn.createStatement();
        }
        catch (SQLException ex) {
            System.out.println("SQLException: " + ex.getMessage());
        }


        SQLBuffer = "DROP TABLE IF EXISTS AnomalyQueryStart_Temp;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "CREATE TEMPORARY TABLE AnomalyQueryStart_Temp AS select a.image AS IMG1, n.file_one AS F1, b.image AS IMG2, n.file_two AS F2, ncd_normal FROM NCD_table AS n JOIN NCD_result AS r JOIN image_snapshot_table AS a JOIN image_snapshot_table AS b ON n.ncd_key = r.ncd_key AND n.file_one = a.item AND n.file_two = b.item AND n.querynumber = " + querynum + " AND ncd_normal <= " + cutoff + "ORDER BY IMG1, F1;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "DROP TABLE IF EXISTS Anomaly_Number_Collabrators_Temp;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "CREATE TEMPORARY TABLE Anomaly_Number_Collabrators_Temp AS select DISTINCT IMG1, F1, COUNT(IMG2) As NUM FROM AnomalyQueryStart_Temp GROUP BY F1;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "DROP TABLE IF EXISTS AnomalyJoin_Temp;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "CREATE TEMPORARY TABLE AnomalyJoin_Temp AS SELECT A.IMG1, A.F1, C.anomaly FROM Anomaly_Number_Collabrators_Temp AS A JOIN AnomalyCurve AS C ON A.NUM = C.number;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "DROP TABLE IF EXISTS AnomalyJoin_Temp2;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "CREATE TEMPORARY TABLE AnomalyJoin_Temp2 AS SELECT A.IMG1, A.F1, C.anomaly FROM Anomaly_Number_Collabrators_Temp AS A JOIN AnomalyCurve AS C ON A.NUM = C.number;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	//Anomaly Temps


	SQLBuffer = "DROP TABLE IF EXISTS Collaborative_Start_Temp;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "CREATE TEMPORARY TABLE Collaborative_Start_Temp AS SELECT DISTINCT S.IMG1 AS IMG1, S.F1 AS F1, J.anomaly AS A1, S.IMG2 AS IMG2, S.F2 AS F2, J2.anomaly AS A2, S.ncd_normal AS NCD, ROUND((J.anomaly + J2.anomaly),3) AS AnomalySUM FROM AnomalyQueryStart_Temp AS S LEFT JOIN AnomalyJoin_Temp AS J USING(IMG1, F1) LEFT JOIN AnomalyJoin_Temp2 AS J2 ON S.IMG2 = J2.IMG1 AND S.F2 = J2.F1;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "DROP TABLE IF EXISTS Collaborative_Result_Temp;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
	SQLBuffer = "CREATE TEMPORARY TABLE Collaborative_Result_Temp AS select IF(MOD(IMG1," +imagetotal+ ") = 0, " +imagetotal+ ", MOD(IMG1," +imagetotal+ ")) AS IMG1, IF(MOD(IMG2," +imagetotal+ ") = 0, " +imagetotal+ ", MOD(IMG2," +imagetotal+ ")) AS IMG2, IF(SUM(AnomalySUM), SUM(AnomalySUM),0.0)  As SUM FROM Collaborative_Start_Temp WHERE IMG2 > IMG1 GROUP BY IMG1,IMG2 ORDER BY SUM;";
        try {
            stmt.executeUpdate(SQLBuffer);
        }
          catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }

    }

    public static void setMaxImages()
    {
        String SQLBuffer = "select count(image) AS NUM FROM image_list_table AS L JOIN query_table AS Q ON Q.casenumber = image_casenumber AND Q.querynumber = "+ global.queryNum + ";";
        ResultSet rs = null;
        Statement stmt = null;

        try {
            stmt = global.conn.createStatement();
        }
        catch (SQLException ex) {
            System.out.println("SQLException: " + ex.getMessage());
        }

        try
        {
            rs = stmt.executeQuery(SQLBuffer);
            rs.next();
            global.imageCount = rs.getInt(1);
            stmt.close();
        }
        catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }

    }

    public static void showGdata(Connection conn)
    {
        String SQLBuffer = null;;
        ResultSet rs = null;
        Statement stmt = null;
        String Collab_table = "Collaborative_Result_Temp";
        int imageOne, imageTwo;
        float collab_score;
        try {
            stmt = conn.createStatement();
        }
        catch (SQLException ex) {
            System.out.println("SQLException: " + ex.getMessage());
        }

        SQLBuffer = "select * FROM " + Collab_table + " WHERE SUM > 0;";

        try
        {
          rs = stmt.executeQuery(SQLBuffer);

          while (rs.next())
          {
            imageOne = rs.getInt(1);
            imageTwo = rs.getInt(2);
            collab_score = rs.getFloat(3);

            System.out.println("IMG" + imageOne + "\tIMG" + imageTwo +"\t"+collab_score);
          }
          if (stmt != null) { stmt.close(); }
        }
        catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }

    }



    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        jLabel1 = new javax.swing.JLabel();
        jLabel2 = new javax.swing.JLabel();
        jPanelCutoff = new javax.swing.JPanel();
        jGraphCutoffLabel = new javax.swing.JLabel();
        jPanelCurve = new javax.swing.JPanel();
        jAnomalyGraphLabel = new javax.swing.JLabel();
        jPanel3 = new javax.swing.JPanel();
        QueryNumber = new javax.swing.JTextField();
        jLabel3 = new javax.swing.JLabel();
        jExitButton = new javax.swing.JButton();
        jUpdateButton = new javax.swing.JButton();
        jCurveSlider = new javax.swing.JSlider();
        jCutoffSlider = new javax.swing.JSlider();
        jCutoffLabel = new javax.swing.JLabel();
        jCurveLabel = new javax.swing.JLabel();
        jSlopeSlider = new javax.swing.JSlider();
        jLabel4 = new javax.swing.JLabel();
        jLabel5 = new javax.swing.JLabel();
        jMaxSlider = new javax.swing.JSlider();
        jSlopeLabel = new javax.swing.JLabel();
        jLabel6 = new javax.swing.JLabel();
        jGraphMaxFileSlider = new javax.swing.JSlider();
        jPanel1 = new javax.swing.JPanel();
        jSQLButton = new javax.swing.JButton();
        jSQLIP = new javax.swing.JTextField();
        jSQLPort = new javax.swing.JTextField();
        jSQLUser = new javax.swing.JTextField();
        jSQLPass = new javax.swing.JPasswordField();
        jLabel8 = new javax.swing.JLabel();
        jLabel9 = new javax.swing.JLabel();
        jLabel10 = new javax.swing.JLabel();
        jLabel11 = new javax.swing.JLabel();

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);

        jLabel1.setText("Similarity Cutoff");

        jLabel2.setText("Anomaly Curve");

        jPanelCutoff.setBackground(new java.awt.Color(204, 255, 204));

        jGraphCutoffLabel.setFont(new java.awt.Font("Lucida Grande", 2, 10)); // NOI18N
        jGraphCutoffLabel.setText("Please Initialize Graph");

        org.jdesktop.layout.GroupLayout jPanelCutoffLayout = new org.jdesktop.layout.GroupLayout(jPanelCutoff);
        jPanelCutoff.setLayout(jPanelCutoffLayout);
        jPanelCutoffLayout.setHorizontalGroup(
            jPanelCutoffLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanelCutoffLayout.createSequentialGroup()
                .addContainerGap()
                .add(jGraphCutoffLabel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 300, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addContainerGap(org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
        );
        jPanelCutoffLayout.setVerticalGroup(
            jPanelCutoffLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(org.jdesktop.layout.GroupLayout.TRAILING, jPanelCutoffLayout.createSequentialGroup()
                .addContainerGap(44, Short.MAX_VALUE)
                .add(jGraphCutoffLabel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 288, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .add(30, 30, 30))
        );

        jPanelCurve.setBackground(new java.awt.Color(204, 255, 204));
        jPanelCurve.setSize(new java.awt.Dimension(310, 310));

        jAnomalyGraphLabel.setFont(new java.awt.Font("Lucida Grande", 2, 10)); // NOI18N
        jAnomalyGraphLabel.setText("Please Initialize Graph");

        org.jdesktop.layout.GroupLayout jPanelCurveLayout = new org.jdesktop.layout.GroupLayout(jPanelCurve);
        jPanelCurve.setLayout(jPanelCurveLayout);
        jPanelCurveLayout.setHorizontalGroup(
            jPanelCurveLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanelCurveLayout.createSequentialGroup()
                .addContainerGap()
                .add(jAnomalyGraphLabel, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 300, Short.MAX_VALUE)
                .add(22, 22, 22))
        );
        jPanelCurveLayout.setVerticalGroup(
            jPanelCurveLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(org.jdesktop.layout.GroupLayout.TRAILING, jPanelCurveLayout.createSequentialGroup()
                .add(33, 33, 33)
                .add(jAnomalyGraphLabel, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 300, Short.MAX_VALUE)
                .add(29, 29, 29))
        );

        jPanel3.setBorder(javax.swing.BorderFactory.createEtchedBorder());

        QueryNumber.setColumns(5);
        QueryNumber.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                QueryNumberActionPerformed(evt);
            }
        });

        jLabel3.setText("Query:");

        jExitButton.setText("Exit");
        jExitButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jExitButtonActionPerformed(evt);
            }
        });

        jUpdateButton.setText("Update");
        jUpdateButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jUpdateButtonActionPerformed(evt);
            }
        });

        org.jdesktop.layout.GroupLayout jPanel3Layout = new org.jdesktop.layout.GroupLayout(jPanel3);
        jPanel3.setLayout(jPanel3Layout);
        jPanel3Layout.setHorizontalGroup(
            jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel3Layout.createSequentialGroup()
                .add(96, 96, 96)
                .add(jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.TRAILING)
                    .add(jPanel3Layout.createSequentialGroup()
                        .add(jLabel3)
                        .add(30, 30, 30))
                    .add(jPanel3Layout.createSequentialGroup()
                        .add(jUpdateButton)
                        .add(18, 18, 18)))
                .add(jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(QueryNumber, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                    .add(jExitButton))
                .add(66, 66, 66))
        );
        jPanel3Layout.setVerticalGroup(
            jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel3Layout.createSequentialGroup()
                .add(33, 33, 33)
                .add(jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(QueryNumber, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                    .add(jLabel3))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                .add(jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(jExitButton)
                    .add(jUpdateButton))
                .add(33, 33, 33))
        );

        jCurveSlider.addChangeListener(new javax.swing.event.ChangeListener() {
            public void stateChanged(javax.swing.event.ChangeEvent evt) {
                jCurveSliderStateChanged(evt);
            }
        });

        jCutoffSlider.addChangeListener(new javax.swing.event.ChangeListener() {
            public void stateChanged(javax.swing.event.ChangeEvent evt) {
                jCutoffSliderStateChanged(evt);
            }
        });

        jCutoffLabel.setText("50%");

        jCurveLabel.setText("00");

        jSlopeSlider.addChangeListener(new javax.swing.event.ChangeListener() {
            public void stateChanged(javax.swing.event.ChangeEvent evt) {
                jSlopeSliderStateChanged(evt);
            }
        });

        jLabel4.setText("Start:");

        jLabel5.setText("Slope:");

        jMaxSlider.setOrientation(javax.swing.JSlider.VERTICAL);
        jMaxSlider.setValue(100);
        jMaxSlider.addChangeListener(new javax.swing.event.ChangeListener() {
            public void stateChanged(javax.swing.event.ChangeEvent evt) {
                jMaxSliderStateChanged(evt);
            }
        });

        jSlopeLabel.setText("00   ");

        jLabel6.setText("NCD/Distance Cutoff:");

        jGraphMaxFileSlider.setOrientation(javax.swing.JSlider.VERTICAL);
        jGraphMaxFileSlider.addChangeListener(new javax.swing.event.ChangeListener() {
            public void stateChanged(javax.swing.event.ChangeEvent evt) {
                jGraphMaxFileSliderStateChanged(evt);
            }
        });

        jPanel1.setBorder(javax.swing.BorderFactory.createTitledBorder("SQL Setup"));

        jSQLButton.setText("Connect");
        jSQLButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jSQLButtonActionPerformed(evt);
            }
        });

        jSQLIP.setText("localhost");

        jSQLPort.setText("2020");

        jSQLUser.setText("snapshot");

        jSQLPass.setText("snapshot");

        jLabel8.setText("Address:");

        jLabel9.setText("Port:");

        jLabel10.setText("User Name:");

        jLabel11.setText("Password:");

        org.jdesktop.layout.GroupLayout jPanel1Layout = new org.jdesktop.layout.GroupLayout(jPanel1);
        jPanel1.setLayout(jPanel1Layout);
        jPanel1Layout.setHorizontalGroup(
            jPanel1Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel1Layout.createSequentialGroup()
                .add(107, 107, 107)
                .add(jSQLButton)
                .addContainerGap(117, Short.MAX_VALUE))
            .add(org.jdesktop.layout.GroupLayout.TRAILING, jPanel1Layout.createSequentialGroup()
                .add(35, 35, 35)
                .add(jPanel1Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(jLabel8)
                    .add(jLabel9)
                    .add(jLabel10)
                    .add(jLabel11))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED, 74, Short.MAX_VALUE)
                .add(jPanel1Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING, false)
                    .add(org.jdesktop.layout.GroupLayout.TRAILING, jSQLPass)
                    .add(org.jdesktop.layout.GroupLayout.TRAILING, jSQLUser)
                    .add(org.jdesktop.layout.GroupLayout.TRAILING, jSQLPort)
                    .add(org.jdesktop.layout.GroupLayout.TRAILING, jSQLIP, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 102, Short.MAX_VALUE))
                .add(37, 37, 37))
        );
        jPanel1Layout.setVerticalGroup(
            jPanel1Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(jPanel1Layout.createSequentialGroup()
                .add(jPanel1Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(jSQLIP, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                    .add(jLabel8))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(jPanel1Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(jSQLPort, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                    .add(jLabel9))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(jPanel1Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(jSQLUser, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                    .add(jLabel10))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(jPanel1Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(jSQLPass, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                    .add(jLabel11))
                .add(18, 18, 18)
                .add(jSQLButton)
                .addContainerGap(org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE))
        );

        org.jdesktop.layout.GroupLayout layout = new org.jdesktop.layout.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(layout.createSequentialGroup()
                .addContainerGap()
                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                        .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                            .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                                    .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                                        .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.TRAILING, false)
                                            .add(jPanel1, 0, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                                            .add(jPanel3, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 332, Short.MAX_VALUE))
                                        .add(18, 18, 18)
                                        .add(jPanelCutoff, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED))
                                    .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                                        .add(jLabel1, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 185, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                        .add(42, 42, 42)))
                                .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                                    .add(jLabel6)
                                    .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                                    .add(jCutoffSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                    .add(50, 50, 50)))
                            .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                                .add(jCutoffLabel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 46, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                .add(111, 111, 111)))
                        .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                            .add(layout.createSequentialGroup()
                                .add(7, 7, 7)
                                .add(jGraphMaxFileSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                .add(11, 11, 11)
                                .add(jPanelCurve, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                .add(9, 9, 9)
                                .add(jMaxSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                            .add(layout.createSequentialGroup()
                                .add(100, 100, 100)
                                .add(jLabel2, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 185, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                            .add(layout.createSequentialGroup()
                                .add(41, 41, 41)
                                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                                    .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.TRAILING)
                                        .add(layout.createSequentialGroup()
                                            .add(jLabel5)
                                            .addPreferredGap(org.jdesktop.layout.LayoutStyle.UNRELATED)
                                            .add(jSlopeSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                                        .add(layout.createSequentialGroup()
                                            .add(jLabel4)
                                            .addPreferredGap(org.jdesktop.layout.LayoutStyle.UNRELATED)
                                            .add(jCurveSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)))
                                    .add(layout.createSequentialGroup()
                                        .add(129, 129, 129)
                                        .add(jCurveLabel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 43, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)))))
                        .add(164, 164, 164))
                    .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                        .add(jSlopeLabel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 45, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                        .add(348, 348, 348))))
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(layout.createSequentialGroup()
                .addContainerGap()
                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(jLabel1)
                    .add(jLabel2))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(layout.createSequentialGroup()
                        .add(jMaxSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 353, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                        .addContainerGap())
                    .add(layout.createSequentialGroup()
                        .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                            .add(jPanelCutoff, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                            .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                                .add(jPanel1, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                                .add(jPanel3, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                            .add(layout.createSequentialGroup()
                                .add(jPanelCurve, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED))
                            .add(layout.createSequentialGroup()
                                .add(jGraphMaxFileSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 352, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)))
                        .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                            .add(layout.createSequentialGroup()
                                .add(18, 18, 18)
                                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.TRAILING)
                                    .add(jCurveSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                                    .add(jLabel4))
                                .add(2, 2, 2)
                                .add(jCurveLabel)
                                .addPreferredGap(org.jdesktop.layout.LayoutStyle.UNRELATED)
                                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                                    .add(jLabel5)
                                    .add(jSlopeSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                                .add(jSlopeLabel)
                                .add(78, 78, 78))
                            .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED, 21, Short.MAX_VALUE)
                                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.TRAILING)
                                    .add(jLabel6)
                                    .add(jCutoffSlider, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                                .add(jCutoffLabel)
                                .add(272, 272, 272)))
                        .add(43, 43, 43))))
        );

        pack();
    }// </editor-fold>//GEN-END:initComponents

    private void QueryNumberActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_QueryNumberActionPerformed
        // TODO add your handling code here:
    }//GEN-LAST:event_QueryNumberActionPerformed

    private void jCutoffSliderStateChanged(javax.swing.event.ChangeEvent evt) {//GEN-FIRST:event_jCutoffSliderStateChanged
        // TODO add your handling code here:
        int cutoff;

        cutoff = jCutoffSlider.getValue();
        jCutoffLabel.setText(cutoff + "%");
        global.cutoff = (float) cutoff / 100;
        
    }//GEN-LAST:event_jCutoffSliderStateChanged

    private void jCurveSliderStateChanged(javax.swing.event.ChangeEvent evt) {//GEN-FIRST:event_jCurveSliderStateChanged
        // TODO add your handling code here:
        int curve;
        curve = jCurveSlider.getValue();
        jCurveLabel.setText(curve + "");
        makeGraphAnomaly();
    }//GEN-LAST:event_jCurveSliderStateChanged

    public void doUpdate()
    {

        //Check if empty - if yes - use default or last.
        if (QueryNumber.getText().equals("")) {
            QueryNumber.setText(global.queryNum + "");
        }

        int QueryNum = Integer.parseInt(QueryNumber.getText());

        if (global.firstrun == true)
        {
            global.queryNum = QueryNum;

            setMaxImages();
            global.AnomalyYarray = new float[ global.imageCount +1 ];
            setGraphSlider();
            jCurveSlider.setMaximum(global.imageCount);
            jCurveSlider.setValue(global.imageCount / 2);
            jCurveLabel.setText( global.imageCount / 2 + "") ;
            jSlopeSlider.setMaximum(global.imageCount * 2);
            jSlopeSlider.setValue(global.imageCount / 2);
            jSlopeLabel.setText(global.imageCount / 2 + "") ;
        }
        //Check if there is a change - if so update and update graph
        if( (QueryNum != global.queryNum) || (global.firstrun == true))
        {
            //Found a change
            global.queryNum = QueryNum;
            makeGraphCutoff();
            makeGraphAnomaly();
            global.firstrun = false;
        }


        //SQL Output - Temp Test Code

        System.out.println("Test Out:" + global.queryNum + " " + global.cutoff + " " + global.imageCount);
        makeAnomalyTable();
        SetupTables(global.conn, global.queryNum , global.cutoff , global.imageCount );
        showGdata(global.conn);
    }
    private void jUpdateButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jUpdateButtonActionPerformed
        doUpdate();
    }//GEN-LAST:event_jUpdateButtonActionPerformed

    private XYDataset createAnomalyDataset() {

        final XYSeries series1 = new XYSeries("Anomaly");
        int x, y, mid, high, slope, yintercept;


        high = jMaxSlider.getValue();
        slope = jSlopeSlider.getValue();
        mid = jCurveSlider.getValue();

        //Clear Array
        for (x = 0; x <= global.imageCount; x++)
        {
            global.AnomalyYarray[x] = 0F;
        }

        //yintercept = ((slope * mid) / high) * -1;
        yintercept = -((slope * mid) - high);
        //UpSwing
        for (x = 0; x <= mid; x++)
        {
            y = slope * x + yintercept;
            if(y >= 0)
            {
                series1.add(x,y);
                global.AnomalyYarray[x] = (float) y / 100;
            }
        }

        slope = slope * -1;
        yintercept = -((slope * mid) - high);
  
        //DownSwing
        for (x = mid+1; x <= global.imageCount; x++)
        {
            y = slope * x + yintercept;
            //System.out.println("Y "+y+" Slope "+slope+" X "+x+" B "+yintercept);
            if(y >= 0)
            {
                series1.add(x,y);
                global.AnomalyYarray[x] = (float) y / 100;

            }
        }


        final XYSeriesCollection dataset = new XYSeriesCollection();
        dataset.addSeries(series1);

        return dataset;
    }

    private void makeGraphAnomaly(){

        final XYDataset dataset = createAnomalyDataset();
        jAnomalyGraphLabel.setText("");
        final JFreeChart chart = ChartFactory.createXYLineChart(
            "Anomaly Curve",         // chart title
            "# of Files",               // domain axis label
            "Value",                  // range axis label
            dataset,                  // data
            PlotOrientation.VERTICAL, // orientation
            false,                     // include legend
            true,                     // tooltips?
            false                     // URLs?
        );

        BufferedImage image = chart.createBufferedImage(300,300);
        jAnomalyGraphLabel.setIcon(new ImageIcon(image));

    }

    private void jExitButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jExitButtonActionPerformed
        try {
        global.conn.close();
        }
         catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
        
        System.exit(0);
    }//GEN-LAST:event_jExitButtonActionPerformed

    private void jMaxSliderStateChanged(javax.swing.event.ChangeEvent evt) {//GEN-FIRST:event_jMaxSliderStateChanged
        makeGraphAnomaly();
    }//GEN-LAST:event_jMaxSliderStateChanged

    private void jSlopeSliderStateChanged(javax.swing.event.ChangeEvent evt) {//GEN-FIRST:event_jSlopeSliderStateChanged
        int Slope;
        Slope = jSlopeSlider.getValue();
        jSlopeLabel.setText(Slope + "");
        makeGraphAnomaly();
    }//GEN-LAST:event_jSlopeSliderStateChanged

    private void jGraphMaxFileSliderStateChanged(javax.swing.event.ChangeEvent evt) {//GEN-FIRST:event_jGraphMaxFileSliderStateChanged
        makeGraphCutoff();
    }//GEN-LAST:event_jGraphMaxFileSliderStateChanged

    private void jSQLButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_jSQLButtonActionPerformed
        // TODO add your handling code here:
       try { if(global.conn != null ) global.conn.close();}
       catch (SQLException ex) {}
       String URL = "jdbc:mysql://" + jSQLIP.getText() + ":" + jSQLPort.getText() + "/SNAPSHOT";
       String password = new String(jSQLPass.getPassword());

       System.out.println("URL "+URL+ " USER "+jSQLUser.getText()+ " PASS "+password);
       global.conn = doConnectsql(URL, jSQLUser.getText(), password);

       doUpdate();
    }//GEN-LAST:event_jSQLButtonActionPerformed

    private void setGraphSlider() {
        String SQLBuffer = "select COUNT(ncd_key) AS Count FROM NCD_table WHERE querynumber = "+ global.queryNum + ";";
        ResultSet rs = null;
        Statement stmt = null;
        int count = 0;

        try {
            stmt = global.conn.createStatement();
        }
        catch (SQLException ex) {
            System.out.println("SQLException: " + ex.getMessage());
        }

        try
        {
            rs = stmt.executeQuery(SQLBuffer);
            rs.next();
            count = rs.getInt("Count");
            stmt.close();
        }
        catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
       jGraphMaxFileSlider.setMaximum((int) count);
       jGraphMaxFileSlider.setValue((int) (count % 1000));

    }
    //Create Dataset for jFreeGraph
    private XYDataset createDataset() {

        ResultSet rs = null;
        Statement stmt = null;
        float value;
        int count;
        int gcutoff = jGraphMaxFileSlider.getValue();


        try {
            stmt = global.conn.createStatement();
        }
        catch (SQLException ex) {
            System.out.println("SQLException: " + ex.getMessage());
        }

        String SQLBuffer = "select R.NCD_normal AS SValue, COUNT(R.ncd_key) AS SCount FROM NCD_result As R Join NCD_table As  T ON R.ncd_key = T.ncd_key WHERE T.querynumber = " + global.queryNum +  " GROUP BY R.NCD_normal  ;";

        final XYSeries series1 = new XYSeries("First");

        try
        {
          rs = stmt.executeQuery(SQLBuffer);

          while (rs.next())
          {
            value = rs.getFloat("SValue");
            count = rs.getInt("SCount") % gcutoff;
            series1.add(value, count);
          }
          if (stmt != null) { stmt.close(); }
        }
        catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }

        final XYSeriesCollection dataset = new XYSeriesCollection();
        dataset.addSeries(series1);

        return dataset;

    }

    private void makeGraphCutoff(){

        final XYDataset dataset = createDataset();

        final JFreeChart chart = ChartFactory.createXYLineChart(
            "Similarity Cutoff",         // chart title
            "Value",               // domain axis label
            "#",                  // range axis label
            dataset,                  // data
            PlotOrientation.VERTICAL, // orientation
            false,                     // include legend
            true,                     // tooltips?
            false                     // URLs?
        );
        BufferedImage image = chart.createBufferedImage(300,300);
        jGraphCutoffLabel.setIcon(new ImageIcon(image));
    }

    public static void makeAnomalyTable()
    {
        int x;
        String SQLBuffer = null;
        ResultSet rs = null;
        Statement stmt = null;


        try {
            stmt = global.conn.createStatement();
        }
        catch (SQLException ex) {
            System.out.println("SQLException: " + ex.getMessage());
        }

        //Drop and Create a new Anomaly table
        try
        {
            SQLBuffer = "DROP table AnomalyCurve;";
            stmt.executeUpdate(SQLBuffer);
            SQLBuffer = "CREATE TABLE AnomalyCurve (number INT, anomaly FLOAT);";
            stmt.executeUpdate(SQLBuffer);
        }
        catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }

        try 
        {
            for (x = 0; x <= global.imageCount; x++)
            {
                if( global.AnomalyYarray[x] > 0)
                {
                    SQLBuffer = "INSERT INTO AnomalyCurve (number, anomaly) VALUES ( "+x+", " + global.AnomalyYarray[x] +");";
                    stmt.executeUpdate(SQLBuffer);
                }
            }
        }//try
        catch (SQLException ex ) {
             System.out.println("SQLException: " + ex.getMessage());
        }
    }

    /**
    * @param args the command line arguments
    */
    public static void main(String args[]) {

        System.out.println("Test of SQL.");
      
     
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                new myMainForm().setVisible(true);
            }
        });
    }

    public static class global{
        public static String connUrl = "jdbc:mysql://localhost:2020/SNAPSHOT";
        public static String user = "snapshot";
        public static String pass = "snapshot";
        public static Connection conn = null;
        public static int imageCount = 20;
        public static int queryNum = 1;
        public static float cutoff = .5F;
        public static boolean firstrun = true;
        public static int graphCutoff = 500;
        public static float[] AnomalyYarray;

    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JTextField QueryNumber;
    private javax.swing.JLabel jAnomalyGraphLabel;
    private javax.swing.JLabel jCurveLabel;
    private javax.swing.JSlider jCurveSlider;
    private javax.swing.JLabel jCutoffLabel;
    private javax.swing.JSlider jCutoffSlider;
    private javax.swing.JButton jExitButton;
    private javax.swing.JLabel jGraphCutoffLabel;
    private javax.swing.JSlider jGraphMaxFileSlider;
    private javax.swing.JLabel jLabel1;
    private javax.swing.JLabel jLabel10;
    private javax.swing.JLabel jLabel11;
    private javax.swing.JLabel jLabel2;
    private javax.swing.JLabel jLabel3;
    private javax.swing.JLabel jLabel4;
    private javax.swing.JLabel jLabel5;
    private javax.swing.JLabel jLabel6;
    private javax.swing.JLabel jLabel8;
    private javax.swing.JLabel jLabel9;
    private javax.swing.JSlider jMaxSlider;
    private javax.swing.JPanel jPanel1;
    private javax.swing.JPanel jPanel3;
    private javax.swing.JPanel jPanelCurve;
    private javax.swing.JPanel jPanelCutoff;
    private javax.swing.JButton jSQLButton;
    private javax.swing.JTextField jSQLIP;
    private javax.swing.JPasswordField jSQLPass;
    private javax.swing.JTextField jSQLPort;
    private javax.swing.JTextField jSQLUser;
    private javax.swing.JLabel jSlopeLabel;
    private javax.swing.JSlider jSlopeSlider;
    private javax.swing.JButton jUpdateButton;
    // End of variables declaration//GEN-END:variables

}
