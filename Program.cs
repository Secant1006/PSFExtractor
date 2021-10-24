using System;
using System.Collections;
using System.IO;

namespace PSFExtractor
{
    class Program
    {
        static int Main(string[] args)
        {
            Console.WriteLine("PSFExtractor v1.08 (Oct 24 2021) by th1r5bvn23 & abbodi1406\nVisit https://www.betaworld.cn/\n");
            string CABFileName;
            string PSFFileName;
            string XMLFileName;
            string DirectoryName;
            switch (args.Length)
            {
                case 1:
                    CABFileName = args[0];
                    if (!File.Exists(CABFileName))
                    {
                        PrintError(1);
                        return 1;
                    }
                    DirectoryName = CABFileName.Substring(0, CABFileName.LastIndexOf('.'));
                    PSFFileName = DirectoryName + ".psf";
                    if (!File.Exists(PSFFileName))
                    {
                        PrintError(3);
                        return 1;
                    }
                    try
                    {
                        Directory.CreateDirectory(DirectoryName);
                    }
                    catch (Exception e)
                    {
                        PrintError(2);
                        return 1;
                    }
                    try
                    {
                        PreProcessing.PreProcess.Process(CABFileName, DirectoryName);
                    }
                    catch (Exception e)
                    {
                        PrintError(4);
                        return 1;
                    }
                    XMLFileName = DirectoryName + Path.DirectorySeparatorChar + "express.psf.cix.xml";
                    break;
                case 4:
                    if (!args[0].Equals("-m"))
                    {
                        PrintHelp();
                        return 1;
                    }
                    PSFFileName = args[1];
                    if (!File.Exists(PSFFileName))
                    {
                        PrintError(1);
                        return 1;
                    }
                    XMLFileName = args[2];
                    if (!File.Exists(XMLFileName))
                    {
                        PrintError(1);
                        return 1;
                    }
                    DirectoryName = args[3];
                    try
                    {
                        Directory.CreateDirectory(DirectoryName);
                    }
                    catch (Exception e)
                    {
                        PrintError(2);
                        return 1;
                    }
                    break;
                default:
                    PrintHelp();
                    return 0;
            }
            SplitPSF.DeltaFileList.List = new ArrayList();
            try
            {
                SplitPSF.GenerateFileList.Generate(XMLFileName);
            }
            catch(Exception e)
            {
                PrintError(5);
                return 1;
            }
            try
            {
                SplitPSF.SplitOutput.WriteOutput(PSFFileName, DirectoryName);
            }
            catch(Exception e)
            {
                Console.WriteLine(e.ToString());
                PrintError(6);
                return 1;
            }
            PrintError(0);
            return 0;
        }

        static void PrintHelp()
        {
            //                |---------------------------------------80---------------------------------------|
            Console.WriteLine("Usage: PSFExtractor.exe <CAB file>\n" +
                              "       PSFExtractor.exe -m <PSF file> <XML file> <destination>\n\n" +
                              "    Auto mode      Auto detect CAB file and PSF file. Works only for Windows 10+.\n" +
                              "    <CAB file>     Path to CAB file. Use only in auto mode. Need corresponding\n" +
                              "                   PSF file with the same name in the same location.\n" +
                              "    -m             Specify PSF file and its descriptive XML file manually. Works\n" +
                              "                   for any update.\n" +
                              "    <PSF file>     Path to PSF file. Use only in manual mode.\n" +
                              "    <XML file>     Path to XML file. Use only in manual mode.\n" +
                              "    <destination>  Path to output folder. If the folder does not exist, it will\n" +
                              "                   be created. Use only in manual mode.\n");
        }

        static void PrintError(int ErrorCode)
        {
            switch (ErrorCode)
            {
                case 0:
                    Console.WriteLine("Finished!");
                    break;
                case 1:
                    Console.WriteLine("Error: file does not exist.");
                    break;
                case 2:
                    Console.WriteLine("Error: failed to create directory.");
                    break;
                case 3:
                    Console.WriteLine("Error: no corresponding PSF file.");
                    break;
                case 4:
                    Console.WriteLine("Error: expand failed.");
                    break;
                case 5:
                    Console.WriteLine("Error: invalid XML file.");
                    break;
                case 6:
                    Console.WriteLine("Error: failed to write output file.");
                    break;
            }
        }
    }
}
