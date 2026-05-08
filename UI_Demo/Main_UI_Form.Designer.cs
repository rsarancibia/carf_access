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
            pbUserPhoto = new PictureBox();
            lblDedo2 = new Label();
            lblDedo1 = new Label();
            btnDoMocF2 = new Button();
            btnLed = new Button();
            btnTag_Get_Dg = new Button();
            lblQR_FechNac = new Label();
            lblQR_FechExp = new Label();
            lblQR_NroDoc = new Label();
            btnCnnnect_AppIcao_1 = new Button();
            btnCnnnect_AppIcao_0 = new Button();
            GetLicence = new Button();
            button1 = new Button();
            btnClearLog = new Button();
            lblTipo_Cedula_Val = new Label();
            lblTipo_Cedula = new Label();
            btnDoMocF1 = new Button();
            pbDisplay = new PictureBox();
            btnCapture_Finger = new Button();
            pbSignature = new PictureBox();
            tableLayoutPanel1.SuspendLayout();
            panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)pbUserPhoto).BeginInit();
            ((System.ComponentModel.ISupportInitialize)pbDisplay).BeginInit();
            ((System.ComponentModel.ISupportInitialize)pbSignature).BeginInit();
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
            tableLayoutPanel1.RowStyles.Add(new RowStyle(SizeType.Percent, 50.35971F));
            tableLayoutPanel1.RowStyles.Add(new RowStyle(SizeType.Percent, 49.64029F));
            tableLayoutPanel1.Size = new Size(1382, 814);
            tableLayoutPanel1.TabIndex = 0;
            // 
            // richTB_Log
            // 
            richTB_Log.Dock = DockStyle.Fill;
            richTB_Log.Location = new Point(3, 412);
            richTB_Log.Name = "richTB_Log";
            richTB_Log.Size = new Size(1376, 399);
            richTB_Log.TabIndex = 0;
            richTB_Log.Text = "";
            // 
            // panel1
            // 
            panel1.Controls.Add(pbSignature);
            panel1.Controls.Add(pbUserPhoto);
            panel1.Controls.Add(lblDedo2);
            panel1.Controls.Add(lblDedo1);
            panel1.Controls.Add(btnDoMocF2);
            panel1.Controls.Add(btnLed);
            panel1.Controls.Add(btnTag_Get_Dg);
            panel1.Controls.Add(lblQR_FechNac);
            panel1.Controls.Add(lblQR_FechExp);
            panel1.Controls.Add(lblQR_NroDoc);
            panel1.Controls.Add(btnCnnnect_AppIcao_1);
            panel1.Controls.Add(btnCnnnect_AppIcao_0);
            panel1.Controls.Add(GetLicence);
            panel1.Controls.Add(button1);
            panel1.Controls.Add(btnClearLog);
            panel1.Controls.Add(lblTipo_Cedula_Val);
            panel1.Controls.Add(lblTipo_Cedula);
            panel1.Controls.Add(btnDoMocF1);
            panel1.Controls.Add(pbDisplay);
            panel1.Controls.Add(btnCapture_Finger);
            panel1.Dock = DockStyle.Fill;
            panel1.Location = new Point(3, 3);
            panel1.Name = "panel1";
            panel1.Size = new Size(1376, 403);
            panel1.TabIndex = 1;
            // 
            // pbUserPhoto
            // 
            pbUserPhoto.BorderStyle = BorderStyle.FixedSingle;
            pbUserPhoto.Location = new Point(625, 9);
            pbUserPhoto.Name = "pbUserPhoto";
            pbUserPhoto.Size = new Size(329, 367);
            pbUserPhoto.SizeMode = PictureBoxSizeMode.Zoom;
            pbUserPhoto.TabIndex = 18;
            pbUserPhoto.TabStop = false;
            // 
            // lblDedo2
            // 
            lblDedo2.AutoSize = true;
            lblDedo2.Location = new Point(503, 140);
            lblDedo2.Name = "lblDedo2";
            lblDedo2.Size = new Size(38, 15);
            lblDedo2.TabIndex = 17;
            lblDedo2.Text = "label1";
            // 
            // lblDedo1
            // 
            lblDedo1.AutoSize = true;
            lblDedo1.Location = new Point(350, 140);
            lblDedo1.Name = "lblDedo1";
            lblDedo1.Size = new Size(38, 15);
            lblDedo1.TabIndex = 16;
            lblDedo1.Text = "label1";
            // 
            // btnDoMocF2
            // 
            btnDoMocF2.Location = new Point(460, 169);
            btnDoMocF2.Name = "btnDoMocF2";
            btnDoMocF2.Size = new Size(134, 69);
            btnDoMocF2.TabIndex = 15;
            btnDoMocF2.Text = "Moc F2";
            btnDoMocF2.UseVisualStyleBackColor = true;
            btnDoMocF2.Click += btnDoMocF2_Click;
            // 
            // btnLed
            // 
            btnLed.Location = new Point(247, 33);
            btnLed.Name = "btnLed";
            btnLed.Size = new Size(25, 25);
            btnLed.TabIndex = 14;
            btnLed.Text = "button2";
            btnLed.UseVisualStyleBackColor = true;
            // 
            // btnTag_Get_Dg
            // 
            btnTag_Get_Dg.Location = new Point(399, 278);
            btnTag_Get_Dg.Name = "btnTag_Get_Dg";
            btnTag_Get_Dg.Size = new Size(75, 23);
            btnTag_Get_Dg.TabIndex = 13;
            btnTag_Get_Dg.Text = "Get DG";
            btnTag_Get_Dg.UseVisualStyleBackColor = true;
            btnTag_Get_Dg.Click += btnTag_Get_Dg_Click;
            // 
            // lblQR_FechNac
            // 
            lblQR_FechNac.AutoSize = true;
            lblQR_FechNac.Location = new Point(287, 62);
            lblQR_FechNac.Name = "lblQR_FechNac";
            lblQR_FechNac.Size = new Size(38, 15);
            lblQR_FechNac.TabIndex = 12;
            lblQR_FechNac.Text = "label1";
            // 
            // lblQR_FechExp
            // 
            lblQR_FechExp.AutoSize = true;
            lblQR_FechExp.Location = new Point(287, 96);
            lblQR_FechExp.Name = "lblQR_FechExp";
            lblQR_FechExp.Size = new Size(38, 15);
            lblQR_FechExp.TabIndex = 11;
            lblQR_FechExp.Text = "label1";
            // 
            // lblQR_NroDoc
            // 
            lblQR_NroDoc.AutoSize = true;
            lblQR_NroDoc.Location = new Point(287, 33);
            lblQR_NroDoc.Name = "lblQR_NroDoc";
            lblQR_NroDoc.Size = new Size(38, 15);
            lblQR_NroDoc.TabIndex = 10;
            lblQR_NroDoc.Text = "label1";
            // 
            // btnCnnnect_AppIcao_1
            // 
            btnCnnnect_AppIcao_1.Location = new Point(973, 303);
            btnCnnnect_AppIcao_1.Name = "btnCnnnect_AppIcao_1";
            btnCnnnect_AppIcao_1.Size = new Size(89, 42);
            btnCnnnect_AppIcao_1.TabIndex = 9;
            btnCnnnect_AppIcao_1.Text = "Conn App 1";
            btnCnnnect_AppIcao_1.UseVisualStyleBackColor = true;
            btnCnnnect_AppIcao_1.Click += btnCnnnect_AppIcao_1_Click;
            // 
            // btnCnnnect_AppIcao_0
            // 
            btnCnnnect_AppIcao_0.Location = new Point(973, 244);
            btnCnnnect_AppIcao_0.Name = "btnCnnnect_AppIcao_0";
            btnCnnnect_AppIcao_0.Size = new Size(95, 35);
            btnCnnnect_AppIcao_0.TabIndex = 8;
            btnCnnnect_AppIcao_0.Text = "Conn App 0";
            btnCnnnect_AppIcao_0.UseVisualStyleBackColor = true;
            btnCnnnect_AppIcao_0.Click += btnCnnnect_AppIcao_0_Click;
            // 
            // GetLicence
            // 
            GetLicence.Location = new Point(42, 25);
            GetLicence.Name = "GetLicence";
            GetLicence.Size = new Size(117, 23);
            GetLicence.TabIndex = 7;
            GetLicence.Text = "Get Lic - Copy";
            GetLicence.UseVisualStyleBackColor = true;
            GetLicence.Click += GetLicense_Click;
            // 
            // button1
            // 
            button1.Location = new Point(301, 278);
            button1.Name = "button1";
            button1.Size = new Size(75, 23);
            button1.TabIndex = 6;
            button1.Text = "button1";
            button1.UseVisualStyleBackColor = true;
            // 
            // btnClearLog
            // 
            btnClearLog.Location = new Point(42, 306);
            btnClearLog.Name = "btnClearLog";
            btnClearLog.Size = new Size(111, 39);
            btnClearLog.TabIndex = 5;
            btnClearLog.Text = "Clear Log";
            btnClearLog.UseVisualStyleBackColor = true;
            btnClearLog.Click += btnClearLog_Click;
            // 
            // lblTipo_Cedula_Val
            // 
            lblTipo_Cedula_Val.AutoSize = true;
            lblTipo_Cedula_Val.Font = new Font("Segoe UI", 18F, FontStyle.Regular, GraphicsUnit.Point, 0);
            lblTipo_Cedula_Val.Location = new Point(399, 344);
            lblTipo_Cedula_Val.Name = "lblTipo_Cedula_Val";
            lblTipo_Cedula_Val.Size = new Size(142, 32);
            lblTipo_Cedula_Val.TabIndex = 4;
            lblTipo_Cedula_Val.Text = "Tipo Cèdula";
            // 
            // lblTipo_Cedula
            // 
            lblTipo_Cedula.AutoSize = true;
            lblTipo_Cedula.Font = new Font("Segoe UI", 18F, FontStyle.Regular, GraphicsUnit.Point, 0);
            lblTipo_Cedula.Location = new Point(205, 335);
            lblTipo_Cedula.Name = "lblTipo_Cedula";
            lblTipo_Cedula.Size = new Size(142, 32);
            lblTipo_Cedula.TabIndex = 3;
            lblTipo_Cedula.Text = "Tipo Cèdula";
            // 
            // btnDoMocF1
            // 
            btnDoMocF1.Location = new Point(301, 169);
            btnDoMocF1.Name = "btnDoMocF1";
            btnDoMocF1.Size = new Size(134, 69);
            btnDoMocF1.TabIndex = 2;
            btnDoMocF1.Text = "Moc F1";
            btnDoMocF1.UseVisualStyleBackColor = true;
            btnDoMocF1.Click += btnDoMocF1_Click;
            // 
            // pbDisplay
            // 
            pbDisplay.BorderStyle = BorderStyle.FixedSingle;
            pbDisplay.Location = new Point(42, 72);
            pbDisplay.Name = "pbDisplay";
            pbDisplay.Size = new Size(172, 166);
            pbDisplay.SizeMode = PictureBoxSizeMode.Zoom;
            pbDisplay.TabIndex = 1;
            pbDisplay.TabStop = false;
            // 
            // btnCapture_Finger
            // 
            btnCapture_Finger.Location = new Point(42, 244);
            btnCapture_Finger.Name = "btnCapture_Finger";
            btnCapture_Finger.Size = new Size(117, 47);
            btnCapture_Finger.TabIndex = 0;
            btnCapture_Finger.Text = "Capturar Dedo";
            btnCapture_Finger.UseVisualStyleBackColor = true;
            btnCapture_Finger.Click += btnCapture_Finger_Live_Click;
            // 
            // pbSignature
            // 
            pbSignature.BorderStyle = BorderStyle.FixedSingle;
            pbSignature.Location = new Point(1012, 9);
            pbSignature.Name = "pbSignature";
            pbSignature.Size = new Size(329, 146);
            pbSignature.SizeMode = PictureBoxSizeMode.Zoom;
            pbSignature.TabIndex = 19;
            pbSignature.TabStop = false;
            // 
            // Main_UI_Form
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1382, 814);
            Controls.Add(tableLayoutPanel1);
            Name = "Main_UI_Form";
            Text = "App Demo MOC";
            FormClosing += Main_UI_Form_FormClosing;
            Load += Main_UI_Form_Load;
            tableLayoutPanel1.ResumeLayout(false);
            panel1.ResumeLayout(false);
            panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)pbUserPhoto).EndInit();
            ((System.ComponentModel.ISupportInitialize)pbDisplay).EndInit();
            ((System.ComponentModel.ISupportInitialize)pbSignature).EndInit();
            ResumeLayout(false);
        }

        #endregion

        private TableLayoutPanel tableLayoutPanel1;
        private RichTextBox richTB_Log;
        private Panel panel1;
        private Button btnCapture_Finger;
        private PictureBox pbDisplay;
        private Button btnDoMocF1;
        private Label lblTipo_Cedula;
        private Label lblTipo_Cedula_Val;
        private Button btnClearLog;
        private Button button1;
        private Button GetLicence;
        private Button btnCnnnect_AppIcao_1;
        private Button btnCnnnect_AppIcao_0;
        private Label lblQR_NroDoc;
        private Label lblQR_FechNac;
        private Label lblQR_FechExp;
        private Button btnTag_Get_Dg;
        private Button btnLed;
        private Button btnDoMocF2;
        private Label lblDedo2;
        private Label lblDedo1;
        private Button btnRecover_Img;
        private PictureBox pbUserPhoto;
        private PictureBox pbSignature;
    }
}
