namespace UI_Demo
{
    partial class Main_UI_Form
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            tableLayoutPanel1 = new TableLayoutPanel();
            richTB_Log = new RichTextBox();
            panel1 = new Panel();
            btnClearLog = new Button();
            lblTipo_Cedula_Val = new Label();
            lblTipo_Cedula = new Label();
            btnDoMoc = new Button();
            pbDisplay = new PictureBox();
            btnCapture_Img = new Button();
            button1 = new Button();
            tableLayoutPanel1.SuspendLayout();
            panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)pbDisplay).BeginInit();
            SuspendLayout();
            // 
            // tableLayoutPanel1
            // 
            tableLayoutPanel1.ColumnCount = 1;
            tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            tableLayoutPanel1.Controls.Add(richTB_Log, 0, 1);
            tableLayoutPanel1.Controls.Add(panel1, 0, 0);
            tableLayoutPanel1.Dock = DockStyle.Fill;
            tableLayoutPanel1.Location = new Point(0, 0);
            tableLayoutPanel1.Name = "tableLayoutPanel1";
            tableLayoutPanel1.RowCount = 2;
            tableLayoutPanel1.RowStyles.Add(new RowStyle(SizeType.Percent, 56.6666679F));
            tableLayoutPanel1.RowStyles.Add(new RowStyle(SizeType.Percent, 43.3333321F));
            tableLayoutPanel1.Size = new Size(828, 567);
            tableLayoutPanel1.TabIndex = 0;
            // 
            // richTB_Log
            // 
            richTB_Log.Dock = DockStyle.Fill;
            richTB_Log.Location = new Point(3, 324);
            richTB_Log.Name = "richTB_Log";
            richTB_Log.Size = new Size(822, 240);
            richTB_Log.TabIndex = 0;
            richTB_Log.Text = "";
            // 
            // panel1
            // 
            panel1.Controls.Add(button1);
            panel1.Controls.Add(btnClearLog);
            panel1.Controls.Add(lblTipo_Cedula_Val);
            panel1.Controls.Add(lblTipo_Cedula);
            panel1.Controls.Add(btnDoMoc);
            panel1.Controls.Add(pbDisplay);
            panel1.Controls.Add(btnCapture_Img);
            panel1.Dock = DockStyle.Fill;
            panel1.Location = new Point(3, 3);
            panel1.Name = "panel1";
            panel1.Size = new Size(822, 315);
            panel1.TabIndex = 1;
            // 
            // btnClearLog
            // 
            btnClearLog.Location = new Point(738, 278);
            btnClearLog.Name = "btnClearLog";
            btnClearLog.Size = new Size(75, 23);
            btnClearLog.TabIndex = 5;
            btnClearLog.Text = "Clear";
            btnClearLog.UseVisualStyleBackColor = true;
            btnClearLog.Click += btnClearLog_Click;
            // 
            // lblTipo_Cedula_Val
            // 
            lblTipo_Cedula_Val.AutoSize = true;
            lblTipo_Cedula_Val.Font = new Font("Segoe UI", 18F, FontStyle.Regular, GraphicsUnit.Point, 0);
            lblTipo_Cedula_Val.Location = new Point(597, 96);
            lblTipo_Cedula_Val.Name = "lblTipo_Cedula_Val";
            lblTipo_Cedula_Val.Size = new Size(142, 32);
            lblTipo_Cedula_Val.TabIndex = 4;
            lblTipo_Cedula_Val.Text = "Tipo Cèdula";
            // 
            // lblTipo_Cedula
            // 
            lblTipo_Cedula.AutoSize = true;
            lblTipo_Cedula.Font = new Font("Segoe UI", 18F, FontStyle.Regular, GraphicsUnit.Point, 0);
            lblTipo_Cedula.Location = new Point(416, 96);
            lblTipo_Cedula.Name = "lblTipo_Cedula";
            lblTipo_Cedula.Size = new Size(142, 32);
            lblTipo_Cedula.TabIndex = 3;
            lblTipo_Cedula.Text = "Tipo Cèdula";
            // 
            // btnDoMoc
            // 
            btnDoMoc.Location = new Point(502, 160);
            btnDoMoc.Name = "btnDoMoc";
            btnDoMoc.Size = new Size(134, 69);
            btnDoMoc.TabIndex = 2;
            btnDoMoc.Text = "Moc";
            btnDoMoc.UseVisualStyleBackColor = true;
            btnDoMoc.Click += btnDoMoc_Click;
            // 
            // pbDisplay
            // 
            pbDisplay.BorderStyle = BorderStyle.FixedSingle;
            pbDisplay.Location = new Point(114, 37);
            pbDisplay.Name = "pbDisplay";
            pbDisplay.Size = new Size(172, 166);
            pbDisplay.SizeMode = PictureBoxSizeMode.Zoom;
            pbDisplay.TabIndex = 1;
            pbDisplay.TabStop = false;
            // 
            // btnCapture_Img
            // 
            btnCapture_Img.Location = new Point(143, 209);
            btnCapture_Img.Name = "btnCapture_Img";
            btnCapture_Img.Size = new Size(117, 47);
            btnCapture_Img.TabIndex = 0;
            btnCapture_Img.Text = "Capturar imagen";
            btnCapture_Img.UseVisualStyleBackColor = true;
            btnCapture_Img.Click += button1_Click;
            // 
            // button1
            // 
            button1.Location = new Point(380, 225);
            button1.Name = "button1";
            button1.Size = new Size(75, 23);
            button1.TabIndex = 6;
            button1.Text = "button1";
            button1.UseVisualStyleBackColor = true;
            button1.Click += button1_Click_1;
            // 
            // Main_UI_Form
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(828, 567);
            Controls.Add(tableLayoutPanel1);
            Name = "Main_UI_Form";
            Text = "App Demo MOC";
            FormClosing += Main_UI_Form_FormClosing;
            Load += Main_UI_Form_Load;
            tableLayoutPanel1.ResumeLayout(false);
            panel1.ResumeLayout(false);
            panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)pbDisplay).EndInit();
            ResumeLayout(false);
        }

        #endregion

        private TableLayoutPanel tableLayoutPanel1;
        private RichTextBox richTB_Log;
        private Panel panel1;
        private Button btnCapture_Img;
        private PictureBox pbDisplay;
        private Button btnDoMoc;
        private Label lblTipo_Cedula;
        private Label lblTipo_Cedula_Val;
        private Button btnClearLog;
        private Button button1;
    }
}
