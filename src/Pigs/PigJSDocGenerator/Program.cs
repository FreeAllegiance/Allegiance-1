using PigJSDocGenerator.Entities;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace PigJSDocGenerator
{
    class Program
    {
        static int Main(string[] args)
        {
            //Console.WriteLine(Configuration.OutputJavascriptFile);
            //Console.WriteLine(Configuration.OutputTypescriptDefinitionFile);

            //if (args.Length != 3)
            //{
            //    Console.WriteLine("Usage:");
            //    Console.WriteLine("PigJSDocGenerator <input .idl file> <output .js file> <output .d.ts file>");
            //    return -1;
            //}

            Program program = new Program();
            return program.Run();

        }

        private int Run()
        {
            StringBuilder idlContentsBuilder = new StringBuilder();

            foreach (string inputfile in Configuration.Filenames)
            {
                StreamReader streamReader = new StreamReader(inputfile);
                idlContentsBuilder.Append(streamReader.ReadToEnd());
                streamReader.Close();
            }

            string idlContents = idlContentsBuilder.ToString();


            //StreamReader streamReader = new StreamReader(inputIdlFile);
            //string idlContents = streamReader.ReadToEnd();

            StreamWriter streamWriterJS = new StreamWriter(Configuration.OutputJavascriptFile, false);
            WriteJSDocHeader(streamWriterJS);

            StreamWriter streamWriterTS = new StreamWriter(Configuration.OutputTypescriptDefinitionFile, false);
            WriteTSDocHeader(streamWriterTS);

            FindEnumBlocksAndWriteToFile(streamWriterJS, streamWriterTS, idlContents);

            FindTypedefBlocksAndWriteToFile(streamWriterTS, idlContents);

            FindInterfaceBlocksAndWriteToFile(streamWriterJS, streamWriterTS, idlContents);
            
            streamWriterJS.Close();
            streamWriterTS.Close();

            return 0;
        }

       

        private void FindInterfaceBlocksAndWriteToFile(StreamWriter streamWriterJS, StreamWriter streamWriterTS, string idlContents)
        {
            // (?<block>\[.*?\]\s+(?<blocktype>((interface)|(typedef)|(dispinterface))).*?\{.*?\})
            Regex blockFinder = new Regex(
                  "(?<block>\\[.*?\\]\\s+(?<blocktype>((interface)|(typedef)|(d" +
                  "ispinterface))).*?\\{.*?\\})",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // \[(?<attributes>.*?)].*?interface\s+(?<name>.*?)\s+(\:\s+(?<inherits>.*?)$)?.*?\{(?<lines>.*?)\}
            Regex elementFinder = new Regex(
                  "\\[(?<attributes>.*?)].*?interface\\s+(?<name>.*?)\\s+(\\:\\s" +
                  "+(?<inherits>.*?)$)?.*?\\{(?<lines>.*?)\\}",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // helpstring\(\"(?<helpstring>.*?)\"\)
            Regex helpstringFinder = new Regex(
                  "helpstring\\(\\\"(?<helpstring>.*?)\\\"\\)",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            List<Signature> signatures = new List<Signature>();
            List<Interface> interfaces = new List<Interface>();

            foreach (Match interfaceMatch in blockFinder.Matches(idlContents))
            {
                if (interfaceMatch.Groups["blocktype"].Value.Equals("interface", StringComparison.InvariantCultureIgnoreCase) == false)
                    continue;

                Match elementMatch = elementFinder.Match(interfaceMatch.Groups["block"].Value);
                if (elementMatch.Success == true)
                {
                    string helpstring = String.Empty;

                    Match helpstringMatch = helpstringFinder.Match(elementMatch.Groups["attributes"].Value);
                    if(helpstringMatch.Success == true)
                        helpstring = helpstringMatch.Groups["helpstring"].Value;
                    
                    string name = elementMatch.Groups["name"].Value;
                    string inherits = elementMatch.Groups["inherits"].Value.Trim();

                    interfaces.Add(new Interface()
                    {
                        Name = name,
                        HelpString = helpstring
                    });


                    signatures = ParseInterfaceBlock(elementMatch.Groups["lines"].Value);

                    WriteTSDocInterface(streamWriterTS, helpstring, name, inherits, signatures);

                    //if(name.Equals("IPig") == true)
                    //    WriteJSDocInterface(streamWriterJS, helpstring, name, signatures);
                }
            }

            WriteTSDocCasts(streamWriterTS, interfaces);
            WriteJSDocCasts(streamWriterJS, interfaces);
        }

        private void WriteJSDocCasts(StreamWriter streamWriter, List<Interface> interfaces)
        {
            streamWriter.WriteLine(@"////////////////////////////////////////////////////////////////////////////////////////////////////");
            streamWriter.WriteLine($"// These functions are simple pass-throughs so that your code will work properly at run-time.  ");
            streamWriter.WriteLine($"// This is how you can get intellisense support without having to use typescript to create a pig.");
            streamWriter.WriteLine("");

            streamWriter.WriteLine("var CType = function() { };");

            foreach (var item in interfaces)
            {
                streamWriter.WriteLine($"CType.{item.Name} = function(o) {{ return o; }};");
            }
        }

        private void WriteTSDocCasts(StreamWriter streamWriter, List<Interface> interfaces)
        {
            streamWriter.WriteLine(@"////////////////////////////////////////////////////////////////////////////////////////////////////");
            streamWriter.WriteLine($"// These declarations enable you to convert your JS objects into COM Interfaces so that you can  ");
            streamWriter.WriteLine($"// get intellisense for event handler parameters.");
            streamWriter.WriteLine("");

            streamWriter.WriteLine("interface ICType {");

            foreach (var item in interfaces)
            {
                streamWriter.WriteLine("/**");
                streamWriter.WriteLine($" * Casts the passed object to {item.Name} - {item.HelpString}");
                streamWriter.WriteLine(" */");
                streamWriter.WriteLine($"\t{item.Name}(o: any): {item.Name};");
            }

            streamWriter.WriteLine("}");

            streamWriter.WriteLine("");
            streamWriter.WriteLine("declare const CType: ICType;");
        }

        private void WriteJSDocInterface(StreamWriter streamWriter, string helpstring, string name, List<Signature> signatures)
        {
            streamWriter.WriteLine(@"////////////////////////////////////////////////////////////////////////////////////////////////////");
            streamWriter.WriteLine($"// This is the global pig object. It is created by the COM layer when the pig script is ");
            streamWriter.WriteLine($"// loaded by PigSrv. This is your main hook into all things pig.");
            streamWriter.WriteLine("");
            streamWriter.WriteLine("/**");
            streamWriter.WriteLine(" * " + helpstring);
            streamWriter.WriteLine(" */");

            streamWriter.WriteLine("var Pig = function() { };");
            streamWriter.WriteLine("");

            bool firstFunction = true;

            foreach (var signature in signatures.OrderBy(p => p.IsFunction))
            {
                if (signature.IsFunction == true)  
                {
                    if (firstFunction == true)
                    {
                        streamWriter.WriteLine("");
                        streamWriter.WriteLine("");
                        firstFunction = false;
                    }

                    streamWriter.Write($"Pig.{signature.Name} = function(");

                    bool isFirstParameter = true;
                    foreach (var parameter in signature.Paramaters)
                    {
                        if (isFirstParameter == false)
                            streamWriter.Write(", ");
                        else
                            isFirstParameter = false;

                        streamWriter.Write(parameter.Name);
                    }

                    streamWriter.WriteLine(")");
                    streamWriter.WriteLine("{");

                    streamWriter.Write($"\treturn {signature.Name}(");

                    isFirstParameter = true;
                    foreach (var parameter in signature.Paramaters)
                    {
                        if (isFirstParameter == false)
                            streamWriter.Write(", ");
                        else
                            isFirstParameter = false;

                        streamWriter.Write(parameter.Name);
                    }

                    streamWriter.WriteLine(");");

                    streamWriter.WriteLine("};");
                    streamWriter.WriteLine("");
                }
                else if (signature.SetProperty == true || signature.GetProperty == true)
                {
                    streamWriter.WriteLine($"Pig.{signature.Name} = {signature.Name};");
                }
            }
            streamWriter.WriteLine("");
            streamWriter.WriteLine("");
        }

        private void WriteTSDocInterface(StreamWriter streamWriter, string helpstring, string name, string inherits, List<Signature> signatures)
        {
            streamWriter.WriteLine(@"////////////////////////////////////////////////////////////////////////////////////////////////////");
            streamWriter.WriteLine($"// {name} Interface");
            streamWriter.WriteLine("");
            streamWriter.WriteLine("/**");
            streamWriter.WriteLine(" * " + helpstring);
            streamWriter.WriteLine(" */");
            streamWriter.Write($"interface {name}");

            if (String.IsNullOrWhiteSpace(inherits) == false)
            {
                streamWriter.Write($" extends {inherits}");
            }

            streamWriter.WriteLine("");

            streamWriter.WriteLine("{");

            WriteTSDocGenerateSignatureBlock(streamWriter, signatures, false);

            
            streamWriter.WriteLine("}");
            streamWriter.WriteLine("");

            if (name.Equals("IPigBehavior") == true)
            {
                streamWriter.WriteLine(@"
/**                
 * These are global objects that are injected by COM into the javascript space. Both IPig and IPigBehavior are available, 
 * but because you can get a Pig object from IPigBehavior, I am not going to make constants for those here. 
 */
");
                WriteTSDocGenerateSignatureBlock(streamWriter, signatures, true);
            }
        }

        private void WriteTSDocGenerateSignatureBlock(StreamWriter streamWriter, List<Signature> signatures, bool isGlobal)
        {
            foreach (var signature in signatures)
            {
                if (signature.IsFunction == true)
                {
                    // Helptext block.
                    streamWriter.WriteLine("\t/**");
                    streamWriter.WriteLine($"\t * {signature.HelpString}");
                    streamWriter.WriteLine("\t */");

                    streamWriter.Write($"\t{(isGlobal ? "declare function" : "")} {signature.Name}(");

                    bool firstParameter = true;
                    foreach (var parameter in signature.Paramaters)
                    {
                        if (firstParameter == false)
                            streamWriter.Write(", ");
                        else
                            firstParameter = false;

                        streamWriter.Write($"{parameter.Name}: {parameter.Type}");
                    }

                    if (String.IsNullOrWhiteSpace(signature.ReturnType) == false)
                        streamWriter.WriteLine($"): {signature.ReturnType}{(isGlobal ? ";" : ";")}");
                    else
                        streamWriter.WriteLine($"){(isGlobal ? ";" : ";")}");
                }
                else if (signature.SetProperty == true || signature.GetProperty == true)
                {
                    // Helptext block.
                    streamWriter.WriteLine("\t/**");
                    streamWriter.Write("\t * ");

                    if (signature.SetProperty == false)
                        streamWriter.Write("READONLY: ");

                    streamWriter.WriteLine($"{signature.HelpString}");
                    streamWriter.WriteLine("\t */");

                    // Property declaration block.
                    streamWriter.Write("\t");

                    if (signature.SetProperty == false && isGlobal == false)
                        streamWriter.Write("readonly ");

                    streamWriter.WriteLine($"{(isGlobal ? "declare const " : "")}{signature.Name}: {signature.ReturnType};");
                }
            }
        }

        private List<Signature> ParseInterfaceBlock(string lines)
        {
            List<Signature> returnValue = new List<Signature>();

            // \[(?<attribute>.*?)\]\s+HRESULT\s+(?<signature>.*?);
            Regex signatureFinder = new Regex(
                  "\\[(?<attribute>.*?)\\]\\s+HRESULT\\s+(?<signature>.*?);",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // helpstring\(\"(?<helpstring>.*?)\"\)
            Regex helpstringFinder = new Regex(
                  "helpstring\\(\\\"(?<helpstring>.*?)\\\"\\)",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // (?<name>.*?)\((?<parameters>.*?)\)$
            Regex nameAndParametersFinder = new Regex(
                  "(?<name>.*?)\\((?<parameters>.*?)\\)$",
                RegexOptions.IgnoreCase
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // (?<typeandattributes>.*?)\s+(?<name>[\w\*]+)(,|$)
            Regex typeAndAttributesAndNameFinder = new Regex(
                  "(?<typeandattributes>.*?)\\s+(?<name>[\\w\\*]+)(,|$)",
                RegexOptions.IgnoreCase
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // \[(?<attributes>.*?)\]\s+(?<type>.*?)$
            Regex typeAndAttributesFinder = new Regex(
                  "\\[(?<attributes>.*?)\\]\\s+(?<type>.*?)$",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            //// \[(?<props>.*?)\]
            //Regex propsFinder = new Regex(
            //      "\\[(?<props>.*?)\\]",
            //    RegexOptions.IgnoreCase
            //    | RegexOptions.Singleline
            //    | RegexOptions.CultureInvariant
            //    | RegexOptions.Compiled
            //    );

            // (\[(?<attributes>.*?)\]\s+)?(?<type>.*?)\s+(?<name>[\w\*]+)(,|$)
            Regex parameterParser = new Regex(
                  "(\\[(?<attributes>.*?)\\]\\s+)?(?<type>.*?)\\s+(?<name>[\\w\\*" +
                  "]+)(,|$)",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            Regex attributeFinder = new Regex(
                  "\\s*(?<attribute>.*?)(,|$)",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            foreach (Match signatureMatch in signatureFinder.Matches(lines))
            {
                Signature signature = new Signature();

                string attributeText = signatureMatch.Groups["attribute"].Value;
                string signatureText = signatureMatch.Groups["signature"].Value;

                Match helpstringMatch = helpstringFinder.Match(attributeText);
                if (helpstringMatch.Success == true)
                    signature.HelpString = helpstringMatch.Groups["helpstring"].Value;

                if (attributeText.ToLower().Contains("propget") == true)
                    signature.GetProperty = true;

                if (attributeText.ToLower().Contains("propput") == true)
                    signature.SetProperty = true;

                signature.IsFunction = signature.GetProperty == false && signature.SetProperty == false;

                Match nameAndParametersMatch = nameAndParametersFinder.Match(signatureText);
                if (nameAndParametersMatch.Success == true)
                {
                    signature.Name = nameAndParametersMatch.Groups["name"].Value;
                    signature.ReturnType = "void";

                    //string parameters = nameAndParametersMatch.Groups["parameters"].Value;

                    //foreach (Match propsMatch in propsFinder.Matches(parameters))
                    //{
                    //    if (propsMatch.Groups["props"].Value.Contains(",") == false)
                    //        continue;

                    //    parameters = parameters.Replace(",", "%%%");
                    //}

                    foreach (Match typeAndAttributesAndNameMatch in typeAndAttributesAndNameFinder.Matches(nameAndParametersMatch.Groups["parameters"].Value))
                    {
                        Parameter parameter = new Parameter();

                        string typeAndAttributes = typeAndAttributesAndNameMatch.Groups["typeandattributes"].Value;
                        parameter.Name = typeAndAttributesAndNameMatch.Groups["name"].Value.Replace("*", "");

                        Match typeAndAttributesMatch = typeAndAttributesFinder.Match(typeAndAttributes);
                        if (typeAndAttributesMatch.Success == true)
                        {
                            parameter.Type = FixupJavascriptType(typeAndAttributesMatch.Groups["type"].Value);

                            foreach (Match attributeMatch in attributeFinder.Matches(typeAndAttributesMatch.Groups["attributes"].Value))
                            {
                                parameter.IsOutput |= attributeMatch.Groups["attribute"].Value.Equals("out", StringComparison.InvariantCultureIgnoreCase);
                                parameter.IsReturnValue |= attributeMatch.Groups["attribute"].Value.Equals("retval", StringComparison.InvariantCultureIgnoreCase);

                                // If the property takes in any arguments, then treat it like a method. 
                                signature.IsFunction |= attributeMatch.Groups["attribute"].Value.Equals("in", StringComparison.InvariantCultureIgnoreCase);
                            }
                        }
                        else
                        {
                            parameter.Type = FixupJavascriptType(typeAndAttributes);
                            signature.IsFunction |= true;
                        }

                        if (parameter.IsReturnValue == false && signature.Paramaters.Exists(p => p.Name == parameter.Name) == false)
                            signature.Paramaters.Add(parameter);

                        if (parameter.IsReturnValue == true)
                            signature.ReturnType = parameter.Type;

                    }



                    //foreach (Match parameterMatch in parameterParser.Matches(nameAndParametersMatch.Groups["parameters"].Value))
                    //{
                    //    Parameter parameter = new Parameter();
                        
                    //    foreach (Match attributeMatch in attributeFinder.Matches(parameterMatch.Groups["attributes"].Value))
                    //    {
                    //        parameter.IsOutput |= attributeMatch.Groups["attribute"].Value.Equals("out", StringComparison.InvariantCultureIgnoreCase);
                    //        parameter.IsReturnValue |= attributeMatch.Groups["attribute"].Value.Equals("retval", StringComparison.InvariantCultureIgnoreCase);

                    //        // If the property takes in any arguments, then treat it like a method. 
                    //        signature.IsFunction |= attributeMatch.Groups["attribute"].Value.Equals("in", StringComparison.InvariantCultureIgnoreCase);
                    //    }
                        
                    //    parameter.Name = parameterMatch.Groups["name"].Value;
                    //    parameter.Type = FixupJavascriptType(parameterMatch.Groups["type"].Value);

                    //    if (parameter.IsReturnValue == false)
                    //        signature.Paramaters.Add(parameter);

                    //    signature.ReturnType = parameter.Type;
                    //}
                }

                MergeSignatureIntoSignatureList(returnValue, signature);
            }

            return returnValue;
        }

        //private List<Parameter> ParseParameters(string value)
        //{
        //    List<Parameter> returnValue = new List<Parameter>();

        //    string prop = String.Empty;
        //    string type = String.Empty;
        //    string name = String.Empty;

        //    string buffer = String.Empty;
        //    bool foundType = false;

        //    foreach (char c in value)
        //    {
        //        if (Char.IsWhiteSpace(c))
        //            continue;

        //        if(c == ',')
        //    }
        //}

        private string FixupJavascriptType(string type)
        {
            string returnValue = type.Replace("*", "").Trim();

            switch (returnValue)
            {
                case "BSTR":
                    returnValue = "string";
                    break;

                case "LPCSTR":
                    returnValue = "string";
                    break;

                case "LPCOLESTR":
                    returnValue = "string";
                    break;

                case "VARIANT_BOOL":
                    returnValue = "boolean";
                    break;

                case "BOOL":
                    returnValue = "boolean";
                    break;

                case "DATE":
                    returnValue = "Date";
                    break;

                case "short":
                    returnValue = "number";
                    break;

                case "long":
                    returnValue = "number";
                    break;

                case "int":
                    returnValue = "number";
                    break;

                case "float":
                    returnValue = "number";
                    break;

                case "FLOAT":
                    returnValue = "number";
                    break;

                case "BYTE":
                    returnValue = "number";
                    break;

                case "DWORD":
                    returnValue = "number";
                    break;

                case "double":
                    returnValue = "number";
                    break;

                case "unsigned char":
                    returnValue = "number";
                    break;

                case "char":
                    returnValue = "number";
                    break;

                case "REFIID":
                    returnValue = "Guid";
                    break;


                case "VARIANT":
                    returnValue = "object";
                    break;

                //case "IUnknown":
                //    returnValue = "any";
                //    break;

                case "const void":
                    returnValue = "object";
                    break;

                case "void":
                    returnValue = "object";
                    break;

                case "PigState":
                    returnValue = "Enums.PigState";
                    break;

                case "PigMapType":
                    returnValue = "Enums.PigMapType";
                    break;

                case "PigLobbyMode":
                    returnValue = "Enums.PigLobbyMode";
                    break;

                case "PigThrust":
                    returnValue = "Enums.PigThrust";
                    break;

                case "AGCEventID":
                    returnValue = "number";
                    break;

                //case "AGCEventID":
                //    returnValue = "Enums.AGCEventID";
                //    break;

                case "AGCObjectType":
                    returnValue = "Enums.AGCObjectType";
                    break;

                case "AGCHullAbility":
                    returnValue = "Enums.AGCHullAbility";
                    break;

                case "AGCAsteroidAbility":
                    returnValue = "Enums.AGCAsteroidAbility";
                    break;

                case "AGCAxis":
                    returnValue = "Enums.AGCAxis";
                    break;

                case "AGCEquipmentType":
                    returnValue = "Enums.AGCEquipmentType";
                    break;

                case "AGCChatTarget":
                    returnValue = "Enums.AGCChatTarget";
                    break;

                case "AGCGameStage":
                    returnValue = "Enums.AGCGameStage";
                    break;

                case "FileAttributes":
                    returnValue = "Enums.FileAttributes";
                    break;

                case "AGCTreasureType":
                    returnValue = "Enums.AGCTreasureType";
                    break;
                    
            }

            return returnValue;
        }

        private void MergeSignatureIntoSignatureList(List<Signature> signatures, Signature signature)
        {
            var existingSignature = signatures.FirstOrDefault(p => p.Name == signature.Name);

            if (existingSignature != null)
            {
                existingSignature.GetProperty |= signature.GetProperty;
                existingSignature.SetProperty |= signature.SetProperty;

                if (String.IsNullOrWhiteSpace(existingSignature.HelpString) == true)
                    existingSignature.HelpString = signature.HelpString;

                if (String.IsNullOrWhiteSpace(existingSignature.ReturnType) == true)
                    existingSignature.ReturnType = signature.ReturnType;

                foreach (var parameter in signature.Paramaters)
                {
                    if(existingSignature.Paramaters.Exists(p => p.Name == parameter.Name) == false)
                        existingSignature.Paramaters.Add(parameter);
                }
            }
            else
            {
                signatures.Add(signature);
            }
        }

        private void FindTypedefBlocksAndWriteToFile(StreamWriter tsDocStreamWriter, string idlContents)
        {
            // This one is different from Enum and Interface finders!
            // (?<block>\[.*?\]\s+(?<blocktype>((interface)|(typedef))).*?;)
            Regex blockFinder = new Regex(
                  "(?<block>\\[.*?\\]\\s+(?<blocktype>((interface)|(typedef)))." +
                  "*?;)",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // \[.*?helpstring\(\"(?<helpstring>.*?)\"\).*?\].*?typedef\s+(?<type>.*?)\s+(?<alias>\w+)\s*;
            Regex enumElementFinder = new Regex(
                  "\\[.*?helpstring\\(\\\"(?<helpstring>.*?)\\\"\\).*?\\].*?typ" +
                  "edef\\s+(?<type>.*?)\\s+(?<alias>\\w+)\\s*;",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );


            foreach (Match enumMatch in blockFinder.Matches(idlContents))
            {
                if (enumMatch.Groups["blocktype"].Value.Equals("typedef", StringComparison.InvariantCultureIgnoreCase) == false)
                    continue;

                // The block finding regex is also picking up [v1_enum] types, so we want to skip those.
                if (enumMatch.Groups["block"].Value.Contains("[v1_enum]") == true)
                    continue;

                Match elementMatch = enumElementFinder.Match(enumMatch.Groups["block"].Value);
                if (elementMatch.Success == true)
                {
                    string helpstring = elementMatch.Groups["helpstring"].Value;
                    string type = FixupJavascriptType(elementMatch.Groups["type"].Value);
                    string alias = elementMatch.Groups["alias"].Value;
                    
                    WriteTSDocTypeAlias(tsDocStreamWriter, helpstring, type, alias);
                }
            }
            
            tsDocStreamWriter.WriteLine("");
            tsDocStreamWriter.WriteLine("");
        }

        private void WriteTSDocTypeAlias(StreamWriter streamWriter, string helpstring, string type, string alias)
        {
            streamWriter.WriteLine(@"////////////////////////////////////////////////////////////////////////////////////////////////////");
            streamWriter.WriteLine($"// {alias} Alias");
            streamWriter.WriteLine("");


            streamWriter.WriteLine("\t/**");
            streamWriter.WriteLine("\t * " + helpstring);
            streamWriter.WriteLine("\t */");

            streamWriter.WriteLine($"\ttype {alias} = {type};");
            
            streamWriter.WriteLine("");
        }

        private void FindEnumBlocksAndWriteToFile(StreamWriter jsDocStreamWriter, StreamWriter tsDocStreamWriter, string idlContents)
        {
            //// (?<enumBlock>\[.*?\].*?typedef\s+\[v1_enum\]\s+enum.*?\{.*?\})
            //Regex enumBlockFinder = new Regex(
            //      "(?<enumBlock>\\[.*?\\].*?typedef\\s+\\[v1_enum\\]\\s+enum.*?" +
            //      "\\{.*?\\})",
            //    RegexOptions.IgnoreCase
            //    | RegexOptions.Multiline
            //    | RegexOptions.Singleline
            //    | RegexOptions.CultureInvariant
            //    | RegexOptions.Compiled
            //    );

            // (?<block>\[.*?\]\s+(?<blocktype>((interface)|(typedef))).*?\{.*?\})
            Regex blockFinder = new Regex(
                  "(?<block>\\[.*?\\]\\s+(?<blocktype>((interface)|(typedef)))." +
                  "*?\\{.*?\\})",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // \[.*?helpstring\(\"(?<helpstring>.*?)\"\).*?\].*?typedef\s+\[v1_enum\]\s+enum\s+(?<enumName>.*?)\s+\{(?<enumLines>.*?)\}
            Regex enumElementFinder = new Regex(
                  "\\[.*?helpstring\\(\\\"(?<helpstring>.*?)\\\"\\).*?\\].*?typ" +
                  "edef\\s+\\[v1_enum\\]\\s+enum\\s+(?<enumName>.*?)\\s+\\{(?<e" +
                  "numLines>.*?)\\}",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            // ^\s*(?<name>.*?)(\s*=\s*(?<value>.*?)\s*)?((,)|(//)|$)
            Regex enumValueFinder = new Regex(
                  "^\\s*(?<name>.*?)(\\s*=\\s*(?<value>.*?)\\s*)?((,)|(//)|$)",
                RegexOptions.IgnoreCase
                | RegexOptions.Multiline
                | RegexOptions.Singleline
                | RegexOptions.CultureInvariant
                | RegexOptions.Compiled
                );

            tsDocStreamWriter.WriteLine("declare namespace Enums");
            tsDocStreamWriter.WriteLine("{");

            foreach (Match enumMatch in blockFinder.Matches(idlContents))
            {
                if (enumMatch.Groups["blocktype"].Value.Equals("typedef", StringComparison.InvariantCultureIgnoreCase) == false)
                    continue;

                Match elementMatch = enumElementFinder.Match(enumMatch.Groups["block"].Value);
                if (elementMatch.Success == true)
                {
                    string helpstring = elementMatch.Groups["helpstring"].Value;
                    string enumName = elementMatch.Groups["enumName"].Value;

                    if (enumName.Contains("//") == true)
                        enumName = enumName.Substring(0, enumName.IndexOf("//"));

                    enumName = enumName.Trim();

                    List<EnumValue> enumValues = new List<EnumValue>();
                    int enumValueCounter = 0;
                    foreach (Match enumValueMatch in enumValueFinder.Matches(elementMatch.Groups["enumLines"].Value))
                    {
                        string name = enumValueMatch.Groups["name"].Value;
                        string value = enumValueMatch.Groups["value"].Value;

                        if (String.IsNullOrWhiteSpace(name) == true)
                            continue;

                        if (String.IsNullOrWhiteSpace(value) == true)
                            value = enumValueCounter.ToString();

                        if (value.StartsWith("0x"))
                            enumValueCounter = Int32.Parse(value.Substring(2), NumberStyles.HexNumber);
                        else
                        {
                            int tempNumber;
                            if (Int32.TryParse(value, out tempNumber) == false)
                                enumValueCounter++;
                            else
                                enumValueCounter = tempNumber;
                        }
                        enumValueCounter++;

                        enumValues.Add(new EnumValue()
                        {
                            Name = name,
                            Value = value
                        });
                    }


                    //string[] enumLines = enumValueFinder.Matches(elementMatch.Groups["enumLines"].Value).OfType<Match>().Select(p => p.Groups["enumValue"].Value).ToArray();

                    WriteJSDocEnum(jsDocStreamWriter, helpstring, enumName, enumValues);
                    WriteTSDocEnum(tsDocStreamWriter, helpstring, enumName, enumValues);
                }
            }

            tsDocStreamWriter.WriteLine("}");
            tsDocStreamWriter.WriteLine("");
            tsDocStreamWriter.WriteLine("");
        }

        private void WriteTSDocEnum(StreamWriter streamWriter, string helpstring, string enumName, List<EnumValue> enumValues)
        {
            streamWriter.WriteLine(@"////////////////////////////////////////////////////////////////////////////////////////////////////");
            streamWriter.WriteLine($"// {enumName} Enum");
            streamWriter.WriteLine("");
            

            streamWriter.WriteLine("\t/**");
            streamWriter.WriteLine("\t * " + helpstring);
            streamWriter.WriteLine("\t */");
            
            streamWriter.WriteLine($"\tenum {enumName}");
            streamWriter.WriteLine("\t\t{");

            foreach (var enumValue in enumValues)
            {
                streamWriter.WriteLine($"\t\t{enumValue.Name},");
            }

            streamWriter.WriteLine("\t}");
            
            streamWriter.WriteLine("");
        }

        private void WriteJSDocEnum(StreamWriter streamWriter, string helpstring, string enumName, List<EnumValue> enumValues)
        {
            streamWriter.WriteLine(@"////////////////////////////////////////////////////////////////////////////////////////////////////");
            streamWriter.WriteLine($"// {enumName} Enum");
            streamWriter.WriteLine("");
            streamWriter.WriteLine($"Enums.{enumName} = function () {{ }};");
            streamWriter.WriteLine("");

            foreach (var enumValue in enumValues)
            {
                streamWriter.WriteLine($"Enums.{enumName}.{enumValue.Name} = {enumValue.Value};");
            }

            streamWriter.WriteLine("");
        }

        private void WriteTSDocHeader(StreamWriter streamWriter)
        {
            streamWriter.WriteLine(@"
// ***
// *** 
// *** This is a generated file. Please do not modify this file by hand or your changes will be lost.
// ***
// ***

interface IDispatch { }
interface IDictionary { }
interface IUnknown { }
interface IStream { }
");

            //if (idlFilename.Equals("AGCIDL.idl", StringComparison.InvariantCultureIgnoreCase) == true)
            //{
                //streamWriter.WriteLine("/// <reference path='AGCEventsIDL.d.ts' />");
                streamWriter.WriteLine("");
                streamWriter.WriteLine("type Guid = string;");
            //}

            streamWriter.WriteLine("");
        }

        private void WriteJSDocHeader(StreamWriter streamWriter)
        {
            streamWriter.WriteLine(@"
// ***
// *** 
// *** This is a generated file. Please do not modify this file by hand or your changes will be lost.
// ***
// ***
//
// These values bridge COM Enums defined in IDL into JS compatible enums that can be used for intellisense.
// These values are also available at the global level, but I'm putting them into an Enums wrapper so that 
// intellisense from *IDL.d.ts can be used at design time, but at run time, this file is used to bridge to 
// the real values.


// Enums is the top level namespace. All Enums will go under this namespace. This will avoid collisions
// with global COM variables like ""PigState"" which has the same name as the enum. 

var Enums = function () { };


");

        }
    }
}
