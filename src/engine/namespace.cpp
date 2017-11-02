#include "pch.h"

class NameSpaceInfo : public INameSpaceInfo {
private:
    INameSpace* m_pns;
    ZString     m_str;

public:
    NameSpaceInfo(INameSpace* pns, const ZString& str) :
        m_pns(pns),
        m_str(str)
    {
    }

    INameSpace* GetNameSpace()
    {
        return m_pns;
    }

    const ZString& GetName()
    {
        return m_str;
    }
};

TRef<INameSpaceInfo> CreateNameSpaceInfo(INameSpace* pns, const ZString& str)
{
    return new NameSpaceInfo(pns, str); //Fix memory leak -Imago 8/2/09
}

//////////////////////////////////////////////////////////////////////////////
//
// NameSpace
//
//////////////////////////////////////////////////////////////////////////////

class NameSpaceImpl : public INameSpace {
private:
    //////////////////////////////////////////////////////////////////////////////
    //
    // types
    //
    //////////////////////////////////////////////////////////////////////////////

    class SymbolData {
    public:
        ZString       m_strName;
        TRef<IObject> m_pobject;

        SymbolData(){}

        SymbolData(const ZString& strName, TRef<IObject> pobject) :
            m_strName(strName),
            m_pobject(pobject)
        {
        }

        SymbolData(const SymbolData& data) :
            m_strName(data.m_strName),
            m_pobject(data.m_pobject)
        {
        }
    };

    typedef TList<SymbolData, DefaultNoEquals> SymbolList;

    class TypeData {
    public:
        ZString       m_strName;
        TRef<MDLType> m_ptype;
    };
    
    typedef TList<TypeData, DefaultNoEquals> TypeList;

    //////////////////////////////////////////////////////////////////////////////
    //
    // data members
    //
    //////////////////////////////////////////////////////////////////////////////

    ZString             m_strName;
    INameSpaceList      m_listParents;
    SymbolList          m_listSymbols;
    TypeList            m_listTypes;

    //////////////////////////////////////////////////////////////////////////////
    //
    // methods
    //
    //////////////////////////////////////////////////////////////////////////////

public:
    NameSpaceImpl(const ZString& strName) :
        m_strName(strName)
    {
    }

    NameSpaceImpl(const ZString& strName, INameSpace* pnsParent) :
        m_strName(strName)
    {
        m_listParents.PushFront(pnsParent);
    }

    NameSpaceImpl(const ZString& strName, INameSpaceList& parents) :
        m_strName(strName),
        m_listParents(parents)
    {
        // !!! should make an ordered list of parents so we don't search
        //     any parent more than once.
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // INameSpace methods
    //
    //////////////////////////////////////////////////////////////////////////////

    const ZString& GetName()
    {
        return m_strName;
    }

    void AddMember(const ZString& strName, IObject* pobject)
    {
        ZAssert(FindMemberLocal(strName) == NULL);

        m_listSymbols.PushEnd(SymbolData(strName, pobject));

        //
        // !!! this is a total hack, only do this on Debug builds since it
        //     requires dynamic casting.  MDLC is always built debug so it's
        //     ok.
        //

        #if defined(_DEBUG)
            Value* pvalue = dynamic_cast<Value*>(pobject);

            if (pvalue != NULL) {
                pvalue->SetNameSpaceInfo(CreateNameSpaceInfo(this, strName));
            }
        #endif
    }

    IObject* FindMemberLocal(const ZString& str)
    {
        SymbolList::Iterator iter(m_listSymbols);

        while (!iter.End()) {
            const SymbolData& data = iter.Value();

            if (data.m_strName == str) {
                return data.m_pobject;
            }

            iter.Next();
        }

        return NULL;
    }

    IObject* FindMemberAndNameSpace(const ZString& str, INameSpace*& pns)
    {
        IObject* pobject = FindMemberLocal(str);

        if (pobject != NULL) {
            pns = this;
            return pobject;
        }

        INameSpaceList::Iterator iter(m_listParents);

        while (!iter.End()) {
            pobject = iter.Value()->FindMemberAndNameSpace(str, pns);
            if (pobject != NULL) {
                return pobject;
            }

            iter.Next();
        }

        return NULL;
    }

    IObject* FindMember(const ZString& str)
    {
        INameSpace* pns;

        return FindMemberAndNameSpace(str, pns);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //  Types
    //
    //////////////////////////////////////////////////////////////////////////////

    void AddType(const ZString& strName, MDLType* ptype)
    {
        ZAssert(FindTypeLocal(strName) == NULL);

        m_listTypes.PushEnd();
        m_listTypes.GetEnd().m_strName = strName;
        m_listTypes.GetEnd().m_ptype   = ptype;
    }

    MDLType* FindTypeLocal(const ZString& strName)
    {
        TypeList::Iterator iter(m_listTypes);

        while (!iter.End()) {
            const TypeData& data = iter.Value();

            if (data.m_strName == strName) {
                return data.m_ptype;
            }

            iter.Next();
        }

        return NULL;
    }

    MDLType* FindTypeAndNameSpace(const ZString& str, NameSpaceImpl*& pns)
    {
        MDLType* ptype = FindTypeLocal(str);

        if (ptype != NULL) {
            pns = this;
            return ptype;
        }

        INameSpaceList::Iterator iter(m_listParents);

        while (!iter.End()) {
            NameSpaceImpl* pnsImpl; CastTo(pnsImpl, iter.Value());
            ptype = pnsImpl->FindTypeAndNameSpace(str, pns);
            if (ptype != NULL) {
                return ptype;
            }

            iter.Next();
        }

        return NULL;
    }

    MDLType* FindType(const ZString& strName)
    {
        NameSpaceImpl* pns;

        return FindTypeAndNameSpace(strName, pns);
    }

    void WriteTypeHeader(ZFile* pfile)
    {
        TypeList::Iterator iter(m_listTypes);

        while (!iter.End()) {
            const TypeData& data = iter.Value();
            iter.Next();

            if (data.m_ptype->IsStruct()) {
                pfile->Write(GetStructHeader(data.m_ptype, data.m_strName));
                if (!iter.End()) {
                    pfile->Write(ZString("\n"));
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //  Binary File Writer
    //
    //////////////////////////////////////////////////////////////////////////////

    class MDLBinaryFile : public IMDLBinaryFile {
    private:
        class ImportData {
        public:
			uint32_t  m_indexNameSpaceTable;
            ZString m_str;
        };

        TVector<TRef<INameSpace> > m_pnss;
        TVector<ImportData> m_importDatas;
        TRef<NameSpaceImpl> m_pns;
        TRef<ZFile>         m_pfile;

    public:

        MDLBinaryFile(NameSpaceImpl* pns, ZFile* pfile) :
            m_pns(pns),
            m_pfile(pfile)
        {
        }

        int FindImport(const ZString& str)
        {
            int count = m_importDatas.GetCount();
            for (int index = 0; index < count; index++) {
                if (m_importDatas[index].m_str == str) {
                    return index;
                }
            }

            return -1;
        }

        void AddImport(const ZString& str, INameSpace* pns)
        {
            int count = m_pnss.GetCount();
			// mdvalley: index needs definin' out here
			int index;
            for (index = 0; index < count; index++) {
                if (m_pnss[index] == pns) {
                    break;
                }
            }

            //
            // Do we need to add a new namespace to the namespace import list?
            //

            if (index == count) {
                m_pnss.PushEnd(pns);
            }

            //
            // Add the new import symbol
            //

            m_importDatas.PushEnd();
            ImportData& data = m_importDatas.GetEnd();

            data.m_indexNameSpaceTable = index;
            data.m_str                 = str;
        }

        void AddImport(const ZString& str)
        {
            int index = FindImport(str);

            if (index == -1) {
                INameSpace* pns;
                IObject* pobject = m_pns->FindMemberAndNameSpace(str, pns);

                ZAssert(pobject != NULL);

                if (pns == m_pns) {
                    return;
                }

                AddImport(str, pns);
            }
        }

        //
        // return true if this is really an import
        // or false if it's in our own namespace.
        //

        bool AddImport(INameSpaceInfo* pnsInfo)
        {
            ZString str = pnsInfo->GetName();
            int index = FindImport(pnsInfo->GetName());

            if (index == -1) {
                //
                // Didn't find it search local
                //

                IObject* pobject = m_pns->FindMemberLocal(pnsInfo->GetName());

                if (pobject == NULL) {
                    AddImport(str, pnsInfo->GetNameSpace());
                    return true;
                }
                return false;
            }

            return true;
        }

        void WriteString(const ZString& str)
        {
            m_pfile->Write(ObjectString);
            m_pfile->WriteAlignedString(str);
        }

        void WriteReference(const ZString& str)
        {
            int index = FindImport(str);

            if (index == -1) {
                index = 0;
                SymbolList::Iterator iter(m_pns->m_listSymbols);

                while (!iter.End()) {
                    if (str == iter.Value().m_strName) {
                        m_pfile->Write(ObjectReference);
                        m_pfile->Write(index);
                        return;
                    }
                    index++;
                    iter.Next();
                }
                ZError("WriteReference: can't find " + str);
            } else {
                m_pfile->Write(ObjectImport);
                m_pfile->Write(index);
            }
        }

        void WriteApply()
        {
            m_pfile->Write(ObjectApply);
        }

        ZFile* WriteBinary()
        {
            m_pfile->Write(ObjectBinary);
            return m_pfile;
        }

        void WriteList(uint32_t count)
        {
            m_pfile->Write(ObjectList);
            m_pfile->Write(count);
        }

        void WriteNumber(float value)
        {
            m_pfile->Write(ObjectFloat);
            m_pfile->Write(value);
        }

        void WriteBoolean(bool value)
        {
            if (value) {
                m_pfile->Write(ObjectTrue);
            } else {
                m_pfile->Write(ObjectFalse);
            }
        }

        void WritePair()
        {
            m_pfile->Write(ObjectPair);
        }

        void WriteEnd()
        {
            m_pfile->Write(ObjectEnd);
        }

        void CreateImportTable()
        {
            m_importDatas.SetCount(0);

            SymbolList::Iterator iter(m_pns->m_listSymbols);
            DWORD index = 0;

            while (!iter.End()) {
                Value* pvalue; CastTo(pvalue, iter.Value().m_pobject);

                pvalue->FillImportTable(this);

                iter.Next();
            }
        }

        void Write()
        {
            CreateImportTable();

            //
            // write out the header
            //

            m_pfile->Write(MDLMagic                       );
            m_pfile->Write(MDLVersion                     );
            m_pfile->Write(m_pnss.GetCount()              );  // imported libraries
            m_pfile->Write(m_importDatas.GetCount()       );  // imported symbols
            m_pfile->Write(m_pns->m_listSymbols.GetCount());  // exported symbols
            m_pfile->Write(0                              );  // other defined objects

            //
            // write out used libraries
            //

            {
                int count = m_pnss.GetCount();
                for (int index = 0; index < count; index++) {
                    const ZString& str = m_pnss[index]->GetName();
                    m_pfile->WriteAlignedString(str);
                }
            }

            //
            // write out imported symbols
            //

            {
                int count = m_importDatas.GetCount();
                for (int index = 0; index < count; index++) {
                    const ImportData& data = m_importDatas[index];

                    m_pfile->Write(data.m_indexNameSpaceTable);
                    m_pfile->WriteAlignedString(data.m_str);
                }
            }

            //
            // write out exported symbols
            //

            {
                SymbolList::Iterator iter(m_pns->m_listSymbols);

                while (!iter.End()) {
                    const ZString& str = iter.Value().m_strName;
                    m_pfile->WriteAlignedString(str);
                    iter.Next();
                }
            }

            //
            // write out definitions
            //

            {
                SymbolList::Iterator iter(m_pns->m_listSymbols);
				uint32_t index = 0;

                while (!iter.End()) {
                    Value* pvalue; CastTo(pvalue, iter.Value().m_pobject);

                    m_pfile->Write(index);
                    pvalue->Write(this);
                    m_pfile->Write(ObjectEnd);

                    index++;
                    iter.Next();
                }
            }
        }
    };

    friend class MDLBinaryFile;

    void WriteToBinaryFile(ZFile* pfile)
    {
        TRef<MDLBinaryFile> pmdlFile = new MDLBinaryFile(this, pfile);

        pmdlFile->Write();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Functions for writing to a text file
    //
    //////////////////////////////////////////////////////////////////////////////

    void WriteToTextFile(int indent, ZFile* pfile)
    {
        //
        // write out used libraries
        //

        {
            INameSpaceList::Iterator iter(m_listParents);

            while (!iter.End()) {
                INameSpace* pns = iter.Value();

                pfile->Write(
                      Value::Indent(indent    ) + "use \"" + pns->GetName() + "\";\n"
                );

                iter.Next();
            }

            if (!m_listParents.IsEmpty()) {
                pfile->Write("\n");
            }
        }

        //
        // write out definitions
        //

        {
            SymbolList::Iterator iter(m_listSymbols);

            while (!iter.End()) {
                const SymbolData& data = iter.Value();
                Value* pvalue; CastTo(pvalue, (IObject*)data.m_pobject);

                pfile->Write(
                      Value::Indent(indent    ) + data.m_strName + " = \n"
                    + Value::Indent(indent + 1) + pvalue->GetString(indent + 1) + ";\n\n"
                );

                iter.Next();
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructors
//
//////////////////////////////////////////////////////////////////////////////

TRef<INameSpace> CreateNameSpace(const ZString& strName)
{
    return new NameSpaceImpl(strName);
}

TRef<INameSpace> CreateNameSpace(const ZString& strName, INameSpace* pnsParent)
{
    return new NameSpaceImpl(strName, pnsParent);
}

TRef<INameSpace> CreateNameSpace(const ZString& strName, INameSpaceList& parents)
{
    return new NameSpaceImpl(strName, parents);
}
