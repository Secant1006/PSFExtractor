using System;
using System.IO;

namespace PSFExtractor.PreProcessing
{
    class PreProcess
    {

        public static void Process(string CABFileName, string DirectoryName)
        {
            if(File.Exists(DirectoryName + Path.DirectorySeparatorChar + "express.psf.cix.xml"))
            {
                Console.WriteLine("CAB file is already expanded.");
                return;
            }
            System.Diagnostics.Process ExpandProcess = new System.Diagnostics.Process();
            ExpandProcess.StartInfo.UseShellExecute = false;
            ExpandProcess.StartInfo.CreateNoWindow = true;
            ExpandProcess.StartInfo.FileName = "expand.exe";
            ExpandProcess.StartInfo.Arguments = '\"' + CABFileName + '\"' + " -F:* " + '\"' + DirectoryName + '\"';
            Console.Write("Expanding CAB... ");
            ExpandProcess.Start();
            ExpandProcess.WaitForExit();
            if (ExpandProcess.ExitCode != 0)
            {
                throw new IOException();
            }
            Console.WriteLine("OK");
        }
    }
}
