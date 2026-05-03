namespace LicenseGenerator
{
    partial class MainForm
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
            btn_Select_Private_Key_File = new Button();
            lblPath_Private_Key = new Label();
            label2 = new Label();
            tbLicense_Display = new TextBox();
            bnLicense_Copy = new Button();
            bnLicense_Generate = new Button();
            label3 = new Label();
            lblPath_Public_Key = new Label();
            btn_Select_Public_Key_File = new Button();
            tbHwId_Value_B64 = new TextBox();
            rbIncludeHwId = new RadioButton();
            tbUserLicenseB64 = new TextBox();
            label1 = new Label();
            btnProcessUserB64Info = new Button();
            lblUserHwId = new Label();
            SuspendLayout();
            // 
            // btn_Select_Private_Key_File
            // 
            btn_Select_Private_Key_File.Location = new Point(165, 162);
            btn_Select_Private_Key_File.Name = "btn_Select_Private_Key_File";
            btn_Select_Private_Key_File.Size = new Size(75, 23);
            btn_Select_Private_Key_File.TabIndex = 0;
            btn_Select_Private_Key_File.Text = "Select file";
            btn_Select_Private_Key_File.UseVisualStyleBackColor = true;
            btn_Select_Private_Key_File.Click += btn_Select_Private_Key_File_Click;
            // 
            // lblPath_Private_Key
            // 
            lblPath_Private_Key.AutoSize = true;
            lblPath_Private_Key.Location = new Point(33, 195);
            lblPath_Private_Key.Name = "lblPath_Private_Key";
            lblPath_Private_Key.Size = new Size(31, 15);
            lblPath_Private_Key.TabIndex = 1;
            lblPath_Private_Key.Text = "Path";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new Point(33, 162);
            label2.Name = "label2";
            label2.Size = new Size(117, 15);
            label2.TabIndex = 2;
            label2.Text = "Load RSA Private key";
            // 
            // tbLicense_Display
            // 
            tbLicense_Display.Location = new Point(33, 452);
            tbLicense_Display.Multiline = true;
            tbLicense_Display.Name = "tbLicense_Display";
            tbLicense_Display.Size = new Size(546, 117);
            tbLicense_Display.TabIndex = 3;
            // 
            // bnLicense_Copy
            // 
            bnLicense_Copy.Location = new Point(504, 584);
            bnLicense_Copy.Name = "bnLicense_Copy";
            bnLicense_Copy.Size = new Size(75, 23);
            bnLicense_Copy.TabIndex = 4;
            bnLicense_Copy.Text = "Copy";
            bnLicense_Copy.UseVisualStyleBackColor = true;
            bnLicense_Copy.Click += bnLicense_Copy_Click;
            // 
            // bnLicense_Generate
            // 
            bnLicense_Generate.Location = new Point(504, 396);
            bnLicense_Generate.Name = "bnLicense_Generate";
            bnLicense_Generate.Size = new Size(75, 23);
            bnLicense_Generate.TabIndex = 5;
            bnLicense_Generate.Text = "Generate";
            bnLicense_Generate.UseVisualStyleBackColor = true;
            bnLicense_Generate.Click += bnLicense_Generate_Click;
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new Point(33, 231);
            label3.Name = "label3";
            label3.Size = new Size(114, 15);
            label3.TabIndex = 8;
            label3.Text = "Load RSA Public key";
            // 
            // lblPath_Public_Key
            // 
            lblPath_Public_Key.AutoSize = true;
            lblPath_Public_Key.Location = new Point(33, 264);
            lblPath_Public_Key.Name = "lblPath_Public_Key";
            lblPath_Public_Key.Size = new Size(31, 15);
            lblPath_Public_Key.TabIndex = 7;
            lblPath_Public_Key.Text = "Path";
            // 
            // btn_Select_Public_Key_File
            // 
            btn_Select_Public_Key_File.Location = new Point(165, 231);
            btn_Select_Public_Key_File.Name = "btn_Select_Public_Key_File";
            btn_Select_Public_Key_File.Size = new Size(75, 23);
            btn_Select_Public_Key_File.TabIndex = 6;
            btn_Select_Public_Key_File.Text = "Select file";
            btn_Select_Public_Key_File.UseVisualStyleBackColor = true;
            btn_Select_Public_Key_File.Click += btn_Select_Public_Key_File_Click;
            // 
            // tbHwId_Value_B64
            // 
            tbHwId_Value_B64.Location = new Point(33, 347);
            tbHwId_Value_B64.Name = "tbHwId_Value_B64";
            tbHwId_Value_B64.Size = new Size(335, 23);
            tbHwId_Value_B64.TabIndex = 9;
            // 
            // rbIncludeHwId
            // 
            rbIncludeHwId.AutoSize = true;
            rbIncludeHwId.Location = new Point(33, 322);
            rbIncludeHwId.Name = "rbIncludeHwId";
            rbIncludeHwId.Size = new Size(125, 19);
            rbIncludeHwId.TabIndex = 10;
            rbIncludeHwId.TabStop = true;
            rbIncludeHwId.Text = "Include HwId (B64)";
            rbIncludeHwId.UseVisualStyleBackColor = true;
            // 
            // tbUserLicenseB64
            // 
            tbUserLicenseB64.Location = new Point(33, 38);
            tbUserLicenseB64.Name = "tbUserLicenseB64";
            tbUserLicenseB64.Size = new Size(335, 23);
            tbUserLicenseB64.TabIndex = 11;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(33, 20);
            label1.Name = "label1";
            label1.Size = new Size(100, 15);
            label1.TabIndex = 12;
            label1.Text = "User license input";
            // 
            // btnProcessUserB64Info
            // 
            btnProcessUserB64Info.Location = new Point(407, 37);
            btnProcessUserB64Info.Name = "btnProcessUserB64Info";
            btnProcessUserB64Info.Size = new Size(75, 23);
            btnProcessUserB64Info.TabIndex = 13;
            btnProcessUserB64Info.Text = "Process";
            btnProcessUserB64Info.UseVisualStyleBackColor = true;
            btnProcessUserB64Info.Click += btnProcessUserB64Info_Click;
            // 
            // lblUserHwId
            // 
            lblUserHwId.AutoSize = true;
            lblUserHwId.Location = new Point(156, 73);
            lblUserHwId.Name = "lblUserHwId";
            lblUserHwId.Size = new Size(84, 15);
            lblUserHwId.TabIndex = 14;
            lblUserHwId.Text = "UserInfo_HwId";
            // 
            // MainForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(606, 624);
            Controls.Add(lblUserHwId);
            Controls.Add(btnProcessUserB64Info);
            Controls.Add(label1);
            Controls.Add(tbUserLicenseB64);
            Controls.Add(rbIncludeHwId);
            Controls.Add(tbHwId_Value_B64);
            Controls.Add(label3);
            Controls.Add(lblPath_Public_Key);
            Controls.Add(btn_Select_Public_Key_File);
            Controls.Add(bnLicense_Generate);
            Controls.Add(bnLicense_Copy);
            Controls.Add(tbLicense_Display);
            Controls.Add(label2);
            Controls.Add(lblPath_Private_Key);
            Controls.Add(btn_Select_Private_Key_File);
            Name = "MainForm";
            Text = "Edv License gemerator";
            ResumeLayout(false);
            PerformLayout();
        }

        //private void btn_Select_Public_Key_File_Click(object sender, EventArgs e)
        //{
        //    throw new NotImplementedException();
        //}

        //private void bnLicense_Generate_Click(object sender, EventArgs e)
        //{
        //    throw new NotImplementedException();
        //}

        //private void btn_Select_Private_Key_File_Click(object sender, EventArgs e)
        //{
        //    throw new NotImplementedException();
        //}

        #endregion

        private Button btn_Select_Private_Key_File;
        private Label lblPath_Private_Key;
        private Label label2;
        private TextBox tbLicense_Display;
        private Button bnLicense_Copy;
        private Button bnLicense_Generate;
        private Label label3;
        private Label lblPath_Public_Key;
        private Button btn_Select_Public_Key_File;
        private TextBox tbHwId_Value_B64;
        private RadioButton rbIncludeHwId;
        private TextBox tbUserLicenseB64;
        private Label label1;
        private Button btnProcessUserB64Info;
        private Label lblUserHwId;
    }
}
