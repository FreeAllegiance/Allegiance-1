using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace PigJSDocGenerator
{
    public sealed class Configuration : IConfigurationSectionHandler
    {
        private static readonly Configuration _instance;
        private const String SectionName = "settings";
        private static List<string> _filenames = new List<string>();

        public static List<string> Filenames
        {
            get { return (_filenames); }
        }

        private static string _outputTypescriptDefinitionFile;
        public static string OutputTypescriptDefinitionFile
        {
            get { return (_outputTypescriptDefinitionFile); }
        }

        private static string _outputJavascriptFile;
        public static string OutputJavascriptFile
        {
            get { return (_outputJavascriptFile); }
        }

        static Configuration()
        {
            _instance = (Configuration)ConfigurationManager.GetSection(SectionName);

            if (_instance == null)
            {
                _instance = new Configuration();
                throw new Exception("The '" + SectionName + "' section not provided in the config file.");
            }
        }

        private Configuration()
        {
        }


        public object Create(object parent, object configContext, XmlNode section)
        {
            Configuration returnValue = new Configuration();

            LoadData(section);

            return returnValue;
        }

        private void LoadData(XmlNode section)
        {
            _outputTypescriptDefinitionFile = GetAttributeValue(section, "outputTypescriptDefinitionFile");
            _outputJavascriptFile = GetAttributeValue(section, "outputJavascriptFile");

            XmlNode idlFiles = section.SelectSingleNode("idlFiles");

            foreach (XmlNode idlFile in idlFiles.SelectNodes("idlFile"))
            {
                string filename = GetAttributeValue(idlFile, "filename");
                _filenames.Add(filename);
            }
        }


        private static string GetAttributeValue(XmlNode configurationNode, string attributeName)
        {
            if (configurationNode.Attributes[attributeName] == null)
            {
                throw new Exception("The '" + attributeName +
                                                   "' node not provided in connection node of '" + SectionName +
                                                   "' section.");
            }
            return configurationNode.Attributes[attributeName].InnerText;
        }
    }
}
