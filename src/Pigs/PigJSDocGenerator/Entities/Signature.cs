using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PigJSDocGenerator.Entities
{
    public class Signature
    {
        public bool GetProperty = false;
        public bool SetProperty = false;
        public bool IsFunction = false;
        public string Name;
        public List<Parameter> Paramaters = new List<Parameter>();
        public string ReturnType;
        public string HelpString;
    }
}
