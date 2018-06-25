#include "debrisgeo.h"

#include <camera.h>
#include <model.h>
#include <surface.h>
#include <viewport.h>

//////////////////////////////////////////////////////////////////////////////
//
// DebrisGeo
//
//////////////////////////////////////////////////////////////////////////////

const float maxDistance = 100;


class DebrisGeo : public Geo {
private:
    class DebrisData {
    public:
        Vector m_position;
        Color  m_color;
        float  m_startTime;
    };

    typedef TList<DebrisData> DebrisDataList;

    DebrisDataList m_listData;
    TRef<Surface>  m_psurface;
	float		   m_timeLast; //time last updated
    float          m_time;
    Vector         m_positionLast;
    float          m_distanceTravelledSinceLastParticle;
	float		   m_speed;
	TRef<Number> m_userDistancePerParticle; //user's chosen particle density modifier
	float		   distancePerParticle;

    Number*    GetTime()     { return Number::Cast(GetChild(0));    }
    Viewport*  GetViewport() { return Viewport::Cast(GetChild(1));  }
    Camera*    GetCamera()   { return GetViewport()->GetCamera();   }
    RectValue* GetViewRect() { return GetViewport()->GetViewRect(); }


public:
    DebrisGeo(Modeler* pmodeler, Number* ptime, Viewport* pviewport, TRef<Number> userDistancePerParticle) :
        Geo(ptime, pviewport),
        m_distanceTravelledSinceLastParticle(0.0f)
    {   
		m_timeLast = GetTime()->GetValue();
        m_positionLast = GetCamera()->GetPosition();
        m_psurface = pmodeler->LoadSurface("debris1bmp", true);
		m_userDistancePerParticle = userDistancePerParticle;
    }

    void Evaluate()
    {
        const Rect&       rect     = GetViewRect()->GetValue();
              Camera*     pcamera  = GetCamera();
              float       focus    = pcamera->GetFocus();
              float       rfocus   = 1.0f / focus;
        const Vector&     position = pcamera->GetPosition();
              Orientation orient   = pcamera->GetOrientation();
              Vector      forward  = orient.GetForward();
              Vector      up       = orient.GetUp();
              Vector      right    = orient.GetRight();

        //
        // save the current time
        //

        m_time = GetTime()->GetValue();

        //
        // how far did we move?
        //

        float length = (m_positionLast - position).Length();

		//
		// how fast are we going?
		//
		if (length != 0.0) { //avoid divide-by-zero if ship is not moving
			m_speed = (m_time - m_timeLast) / length;
		} else {
			m_speed = 0;
		}
		m_timeLast = m_time;

		//
		// more stars at lower speed - 0.3 more at 0, ramping down to 0 more at 75mps
		//
		if (m_speed < 75) {
			distancePerParticle = m_userDistancePerParticle->GetValue() - (0.3 - 0.004*m_speed);
		} else {
			distancePerParticle = m_userDistancePerParticle->GetValue();
		}

        if (length > 100 || distancePerParticle == 0 || m_userDistancePerParticle->GetValue() == 0.0f) {
            //
            // we jumped clear everything
            //
			// or debris is turned off - LANS
            //

            m_listData.SetEmpty();
            length = 0;
        } else {
            //
            // remove any debris that are too far away from the ship
            //

            DebrisDataList::Iterator iter(m_listData);

            while (!iter.End()) {
                DebrisData& data = iter.Value();

                float time     = m_time - data.m_startTime;
                float bright   = 1.0f - 0.25f * time * time;
                float distance = (data.m_position - position).LengthSquared();

                if (bright <= 0 || distance > 250000) {
                    iter.Remove();
                } else {
                    iter.Next();
                }
            }

            //
            // if we are moving add new debris
            //

            m_distanceTravelledSinceLastParticle += length;
            while (m_distanceTravelledSinceLastParticle > distancePerParticle) {
                m_listData.PushFront(); 
                DebrisData& data = m_listData.GetFront();

                float angle    = random(0, 2 * pi);
                float distance = maxDistance * sqrt(random(0, 1));
                float radius   = rfocus * distance * sqrt(random(0, 1));
                float c        = radius * cos(angle);
                float s        = radius * sin(angle);

                data.m_position  = 
                      position 
                    + distance * forward
                    + c * right
                    + s * up;

                data.m_color     = 
                    Color(
                        random(0.50f, 0.75f),
                        random(0.75f, 1.00f),
                        random(0.75f, 1.00f)
                    );
                data.m_startTime = m_time;
                m_distanceTravelledSinceLastParticle -= distancePerParticle;
            }
        }

        m_positionLast = position;
    }

    void Render(Context* pcontext)
    {
        const Vector& position = GetCamera()->GetPosition();
        float scale = 0.1f / GetCamera()->GetFocus();

        DebrisDataList::Iterator iter(m_listData);

        while (!iter.End()) {
            DebrisData& data = iter.Value();

            float time     = m_time - data.m_startTime;
            float bright   = 1.0f - 0.25f * time * time;
            float distance = (data.m_position - position).LengthSquared();

            if (distance > 400) {
                pcontext->DrawDecal(
                    m_psurface,
                    bright * data.m_color,
                    data.m_position,
                    Vector::GetZero(),
                    Vector::GetZero(),
                    scale,
                    0
                );
            }

            iter.Next();
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Value members
    //
    //////////////////////////////////////////////////////////////////////////////

    ZString GetFunctionName() { return "DebrisGeo"; }
};

TRef<Geo> CreateDebrisGeo(Modeler* pmodeler, Number* ptime, Viewport* pviewport, TRef<Number> distancePerParticle)
{
    return new DebrisGeo(pmodeler, ptime, pviewport, distancePerParticle);
}
