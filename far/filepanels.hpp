#ifndef __FILEPANELS_HPP__
#define __FILEPANELS_HPP__
/*
filepanels.hpp

�������� ������

*/

/* Revision: 1.04 11.05.2001 $ */

/*
Modify:
  11.05.2001 OT
    ! ��������� Background
  06.05.2001 DJ
    ! �������� #include
  06.05.2001 ��
    ! �������������� Window � Frame :)
  05.05.2001 DJ
    + �������� NWZ
  01.01.2001 tran
      created
*/

#include "frame.hpp"
#include "keybar.hpp"
#include "menubar.hpp"

class Panel;
class CommandLine;

class FilePanels:public Frame
{
  private:
    void DisplayObject();
    int Focus;


  public:

    FilePanels();
    ~FilePanels();

    void Init();

    Panel *LastLeftFilePanel,
          *LastRightFilePanel;

    Panel* CreatePanel(int Type);
    void   DeletePanel(Panel *Deleted);
    Panel* GetAnotherPanel(Panel *Current);
    Panel* ChangePanelToFilled(Panel *Current,int NewType);
    Panel* ChangePanel(Panel *Current,int NewType,int CreateNew,int Force);
    void   SetPanelPositions(int LeftFullScreen,int RightFullScreen);

    void   SetupKeyBar();

    virtual void Show();

    void Redraw();

    Panel *LeftPanel,
          *RightPanel,
          *ActivePanel;

//    CommandLine *CmdLine;
    KeyBar      MainKeyBar;
    MenuBar     TopMenuBar;

    int LastLeftType,
        LastRightType;
    int LeftStateBeforeHide,
        RightStateBeforeHide;
//        HideState;


    int ProcessKey(int Key);
    int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);


    void SetScreenPositions();

    void Update();

    virtual int GetTypeAndName(char *Type,char *Name);
    virtual int GetType() { return MODALTYPE_PANELS; }

    virtual char *GetTypeName(){return "[FilePanels]";};
    virtual void OnChangeFocus(int focus);

    void RedrawKeyBar(); // virtual
    virtual void ShowConsoleTitle();
};

#endif // __FILEPANELS_HPP__
