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
            SuspendLayout();
            // 
            // btn_Select_Private_Key_File
            // 
            btn_Select_Private_Key_File.Location = new Point(166, 36);
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
            lblPath_Private_Key.Location = new Point(34, 69);
            lblPath_Private_Key.Name = "lblPath_Private_Key";
            lblPath_Private_Key.Size = new Size(31, 15);
            lblPath_Private_Key.TabIndex = 1;
            lblPath_Private_Key.Text = "Path";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new Point(34, 36);
            label2.Name = "label2";
            label2.Size = new Size(117, 15);
            label2.TabIndex = 2;
            label2.Text = "Load RSA Private key";
            // 
            // tbLicense_Display
            // 
            tbLicense_Display.Location = new Point(34, 326);
            tbLicense_Display.Multiline = true;
            tbLicense_Display.Name = "tbLicense_Display";
            tbLicense_Display.Size = new Size(546, 117);
            tbLicense_Display.TabIndex = 3;
            // 
            // bnLicense_Copy
            // 
            bnLicense_Copy.Location = new Point(505, 458);
            bnLicense_Copy.Name = "bnLicense_Copy";
            bnLicense_Copy.Size = new Size(75, 23);
            bnLicense_Copy.TabIndex = 4;
            bnLicense_Copy.Text = "Copy";
            bnLicense_Copy.UseVisualStyleBackColor = true;
            bnLicense_Copy.Click += bnLicense_Copy_Click;
            // 
            // bnLicense_Generate
            // 
            bnLicense_Generate.Location = new Point(505, 270);
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
            label3.Location = new Point(34, 105);
            label3.Name = "label3";
            label3.Size = new Size(114, 15);
            label3.TabIndex = 8;
            label3.Text = "Load RSA Public key";
            // 
            // lblPath_Public_Key
            // 
            lblPath_Public_Key.AutoSize = true;
            lblPath_Public_Key.Location = new Point(34, 138);
            lblPath_Public_Key.Name = "lblPath_Public_Key";
            lblPath_Public_Key.Size = new Size(31, 15);
            lblPath_Public_Key.TabIndex = 7;
            lblPath_Public_Key.Text = "Path";
            // 
            // btn_Select_Public_Key_File
            // 
            btn_Select_Public_Key_File.Location = new Point(166, 105);
            btn_Select_Public_Key_File.Name = "btn_Select_Public_Key_File";
            btn_Select_Public_Key_File.Size = new Size(75, 23);
            btn_Select_Public_Key_File.TabIndex = 6;
            btn_Select_Public_Key_File.Text = "Select file";
            btn_Select_Public_Key_File.UseVisualStyleBackColor = true;
            btn_Select_Public_Key_File.Click += btn_Select_Public_Key_File_Click;
            // 
            // tbHwId_Value_B64
            // 
            tbHwId_Value_B64.Location = new Point(34, 221);
            tbHwId_Value_B64.Name = "tbHwId_Value_B64";
            tbHwId_Value_B64.Size = new Size(335, 23);
            tbHwId_Value_B64.TabIndex = 9;
            // 
            // rbIncludeHwId
            // 
            rbIncludeHwId.AutoSize = true;
            rbIncludeHwId.Location = new Point(34, 196);
            rbIncludeHwId.Name = "rbIncludeHwId";
            rbIncludeHwId.Size = new Size(125, 19);
            rbIncludeHwId.TabIndex = 10;
            rbIncludeHwId.TabStop = true;
            rbIncludeHwId.Text = "Include HwId (B64)";
            rbIncludeHwId.UseVisualStyleBackColor = true;
            // 
            // MainForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(606, 509);
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
    }
}
