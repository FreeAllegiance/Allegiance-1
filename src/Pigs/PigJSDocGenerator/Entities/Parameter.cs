using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PigJSDocGenerator.Entities
{
    public class Parameter
    {
        public string Name;
        public string Type;
        public bool IsOutput = false;
        public bool IsReturnValue = false;
    }
}
