/////////////////////////////////////////////////////////////////////////////
// PigStaticSite.h : Declaration of the CPigsThingSite and CPigStaticSite
// classes.
//

#ifndef __PigStaticSite_h__
#define __PigStaticSite_h__

#include <Allegiance.h>

// Copied from FedSrv's implementation.
class   CPigsClusterSite : public ClusterSite
{
public:
	CPigsClusterSite(IclusterIGC * pcluster)
		:
		m_pcluster(pcluster)
	{
	}

	virtual void                    AddScanner(SideID   sid, IscannerIGC* scannerNew)
	{
		assert(sid >= 0);
		assert(sid < c_cSidesMax);
		assert(scannerNew);

		AddIbaseIGC((BaseListIGC*)&(m_scanners[sid]), scannerNew);
	}
	virtual void                    DeleteScanner(SideID   sid, IscannerIGC* scannerOld)
	{
		assert(sid >= 0);
		assert(sid < c_cSidesMax);
		assert(scannerOld);

		DeleteIbaseIGC((BaseListIGC*)&(m_scanners[sid]), scannerOld);
	}
	virtual const ScannerListIGC*      GetScanners(SideID   sid) const
	{
		assert(sid >= 0);
		assert(sid < c_cSidesMax);

		return &(m_scanners[sid]);
	}

	virtual void Terminate()
	{
		ClusterSite::Terminate();
	}

private:
	IclusterIGC * m_pcluster;
	ScannerListIGC  m_scanners[c_cSidesMax];
};


          
// Overload additional ThingSite methods to turn on other client featues. Currently, we are only 
// concerned about side visibility, the other things in ThingSite appear to be graphics related.
class CPigsThingSite : public ThingSite                                    
{ 
private:
	IclusterIGC * currentCluster = nullptr;
	ImodelIGC*   currentModel = nullptr;
	SideVisibility              m_sideVisibility[c_cSidesMax];

public:     

// Taken from trekigc's implementation.
  bool GetSideVisibility(IsideIGC* side)                                   
  {    
	  bool currentEye = false;

	  if (currentCluster == nullptr)
		  return false;

	  CPigsClusterSite *clusterSite = (CPigsClusterSite *) currentCluster->GetClusterSite();
	  const ScannerListIGC * scanners = clusterSite->GetScanners(side->GetObjectID());

	  if (currentCluster == nullptr || clusterSite == nullptr || scanners == nullptr || scanners->n() < 1)
		  return currentEye;

	  IscannerIGC * lastSpotter = m_sideVisibility[side->GetObjectID()].pLastSpotter();

	  if (lastSpotter && lastSpotter->InScannerRange(currentModel))
	  {
		  currentEye = true;
	  }
	  else
	  {
		  //do it the hard way
		  for (ScannerLinkIGC* l = currentCluster->GetClusterSite()->GetScanners(side->GetObjectID())->first(); (l != NULL); l = l->next())
		  {
			  IscannerIGC*   s = l->data();

			  if (s->InScannerRange(currentModel))
			  {
				  currentEye = true;
				  m_sideVisibility[side->GetObjectID()].pLastSpotter(s);
				  break;
			  }
		  }
	  }

	  return currentEye;
  }   

  void        SetCluster(ImodelIGC*   pmodel, IclusterIGC* pcluster)
  {
	  currentCluster = pcluster;
	  currentModel = pmodel;
  }
};                                                                       



/////////////////////////////////////////////////////////////////////////////
// CPigStaticSite - Note, this is decoy. The real ones are in Pig.cpp. 
//
class CPigStaticSite : public IIgcSite
{
// IIgcSite Overrides
public:
  virtual TRef<ThingSite> CreateThingSite(ImodelIGC* pModel)
  {
    return new CPigsThingSite();
  }

  virtual TRef<ClusterSite> CreateClusterSite(IclusterIGC* pCluster)
  {
	  return new CPigsClusterSite(pCluster);
  }
};


/////////////////////////////////////////////////////////////////////////////

#endif !__PigStaticSite_h__



