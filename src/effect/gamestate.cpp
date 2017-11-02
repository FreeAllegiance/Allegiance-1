#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Game State
//
//////////////////////////////////////////////////////////////////////////////

class GameStateContainerImpl : public GameStateContainer {
private:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Static members
    //
    //////////////////////////////////////////////////////////////////////////////

    static Color s_colorLight;
    static Color s_colorDark;
    static Color s_colorBackground;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Members
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<Modeler>     m_pmodeler;
    TRef<ColumnPane>  m_pcolTiles;
    TRef<Image>       m_pimage;
    TRef<StringPane>  m_pstringPaneTime;
    TRef<IEngineFont> m_pfontTitles;
    int               m_secondsOld;
    TRef<ButtonPane>  m_pbuttonClose;

public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

    GameStateContainerImpl(
        Modeler* pmodeler,
        IEngineFont* pfontTitles,
        IEngineFont* pfontTime,
        ButtonPane*  pbutton
    ) :
        m_pmodeler(pmodeler),
        m_pfontTitles(pfontTitles),
        m_secondsOld(-1),
        m_pbuttonClose(pbutton)
    {
        //
        // The column all the game state tiles will live in
        //

        m_pcolTiles = new ColumnPane();

        //
        // left and right borders
        //

        TRef<Pane> ppaneInfo =
            new BorderPane(
                1,
                0,
                s_colorLight,
                new BorderPane(
                    0,
                    0,
                    s_colorDark,
                    new BorderPane(
                        0,
                        0,
                        s_colorLight,
                        new BorderPane(
                            0,
                            0,
                            s_colorBackground,
                            m_pcolTiles
                        )
                    )
                )
            );

        //
        // The footer
        //

        TRef<Pane> ppaneFooter = new ImagePane(pmodeler->LoadImage("gamestatefooterbmp", true));

        m_pstringPaneTime = new StringPane(ZString("2:00"), pfontTime);
        m_pstringPaneTime->SetOffset(WinPoint(50, 3));
        m_pstringPaneTime->SetTextColor(Color::White());
        ppaneFooter->InsertAtBottom(m_pstringPaneTime);

        //
        // Put it all together
        //

        TRef<ColumnPane> pcol = new ColumnPane();
		
        pcol->InsertAtBottom(new ImagePane(pmodeler->LoadImage("gamestateheaderbmp", true)));
		pcol->InsertAtBottom(new FillPane(WinPoint(0, 1), s_colorLight));
        pcol->InsertAtBottom(ppaneInfo);
        pcol->InsertAtBottom(ppaneFooter);

        //
        // Turn the pane into an image
        //

        m_pimage = 
            CreatePaneImage(
                pmodeler->GetEngine(),
                true,
                pcol
            );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // GameStateContainer methods
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<StringPane> AddGameStateTile(
        const ZString& strTitle, 
        Pane*          ppane
    ) {
        TRef<StringPane> pstringPane = new StringPane(ZString(), m_pfontTitles);

        m_pcolTiles->InsertAtBottom(
            new BorderPane(
                0, 
                1,
                s_colorBackground,
                new ColumnPane(
                    new ImagePane(m_pmodeler->LoadImage(strTitle, true)),
					new BorderPane(
						3,
						1,
						s_colorBackground,
						new ColumnPane(
							pstringPane,
							ppane
					)),
                    ppane
                )
            )
        );

        m_pcolTiles->InsertAtBottom(new FillPane(WinPoint(0, 1), s_colorLight));
        return pstringPane;
    }

    TRef<Image> GetImage()
    {
        return m_pimage;
    }

    TRef<IEngineFont> GetFont()
    {
        return m_pfontTitles;
    }

    void SetTimeElapsed(int seconds)
    {
        if (seconds != m_secondsOld)
        {
            m_secondsOld = seconds;
            if (seconds < 0)
            {
                m_pstringPaneTime->SetString("Game Starting");
            }
            else
            {
                int minutes = seconds / 60;
                seconds -= minutes * 60;

                m_pstringPaneTime->SetString(
                    "Time Elapsed: " + ZString(minutes) + ":" 
                    + ((seconds > 9) ? ZString(seconds) : ("0" + ZString(seconds)))
					);
            }
        }
    }

    void SetTimeRemaining(int seconds)
    {
        if (seconds != m_secondsOld)
        {
            m_secondsOld = seconds;
            if (seconds < 0)
            {
                m_pstringPaneTime->SetString("Game Over");
            }
            else
            {
                int minutes = seconds / 60;
                seconds -= minutes * 60;

                m_pstringPaneTime->SetString(
                    "Time Remaining: " + ZString(minutes) + ":" 
                    + ((seconds > 9) ? ZString(seconds) : ("0" + ZString(seconds)))
					);
            }
        }
    }

    IEventSource* GetCloseEvent()
    {
        return m_pbuttonClose->GetEventSource();
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Static members
//
//////////////////////////////////////////////////////////////////////////////

Color GameStateContainerImpl::s_colorLight(
    255.0f / 255.0f, 
    255.0f / 255.0f, 
    255.0f / 255.0f
);

Color GameStateContainerImpl::s_colorDark(
     21.0f / 255.0f, 
     11.0f / 255.0f, 
     19.0f / 255.0f
);

Color GameStateContainerImpl::s_colorBackground(
   13.0f / 255.0f, 13.0f / 255.0f, 13.0f / 255.0f
);

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<GameStateContainer> CreateGameStateContainer(
    Modeler*     pmodeler,
    IEngineFont* pfontTitles,
    IEngineFont* pfontTime,
    ButtonPane*  pbutton
) {
    return new GameStateContainerImpl(pmodeler, pfontTitles, pfontTime, pbutton);
}



