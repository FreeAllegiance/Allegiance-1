using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace PigClient
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        PigsLib.PigSession session = null;
        private int PigSrvLatestProcessId = 0;
        private Process pigSrv;
        private IObservable<Unit> refreshTicker;
        private IDisposable addPigSub;
        private IDisposable refreshSub;
        private IDisposable fixCrashedPigSrv;
        bool _refreshEnabled = true;
        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = this;
            refreshTicker = Observable.Interval(TimeSpan.FromSeconds(1)).Select(x => Unit.Default);

            addPigSub = Observable.Interval(TimeSpan.FromSeconds(45)).SkipWhile(x =>
            {
                return session == null ||
                        pigInfos.Count == 0 ||
                        pigInfos.Count(p => p.Pig.PigStateName == "Flying" || p.Pig.PigStateName == "Docked") == 0;
            })
            .Select(x =>
            {
                return pigInfos.FirstOrDefault(p => p.Pig.PigStateName == "Flying" || p.Pig.PigStateName == "Docked");
            })
            .Where(pig => pig != null && pig.Pig != null)
            .Select(pig =>
            {
                return pig.Pig;
            })
            .Select(pig =>
            {
                var friendsToAdd = 0;
                var myTeamId = pig.Ship.Team.ObjectID;
                var friendlyCount = 0;
                var hostileCount = 0;
                var shipCount = pig.Game.Ships.Count;
                try
                {

                    for (var i = 0; i < shipCount; i++)
                    {
                        if (pig.Game.Ships[i].Team.ObjectID == myTeamId)
                        {
                            friendlyCount++;
                        }
                        else
                        {
                            hostileCount++;
                        }
                    }

                    var idealRatio = _minRatio;
                    var decreaseRatio = _maxRatio;
                    if (hostileCount > 0 && friendlyCount / hostileCount < idealRatio || friendlyCount < _minPigs)
                    {
                        friendsToAdd++;
                    }
                    else if (hostileCount > 0 && friendlyCount > _minPigs && friendlyCount / hostileCount > decreaseRatio )
                    {
                        friendsToAdd--;
                    }

                } catch
                {

                }
                return friendsToAdd;
            })
            .ObserveOnDispatcher(System.Windows.Threading.DispatcherPriority.Normal)
            .Subscribe(x =>
            {
                if (x > 0)
                {
                    log("Adding pig to keep it painful");
                    CreatePig_Click(null, null);
                }
                else if (x < 0)
                {
                    var Rand = new Random(Environment.TickCount);
                    var toRemove = pigInfos.Skip(1) // don't want to empty the team
                                    .Where(p => p.Pig.PigStateName == "Flying" || p.Pig.PigStateName == "Docked")
                                    .OrderBy(p => Rand.Next())
                                    .FirstOrDefault();
                    if (toRemove != null)
                    {
                        toRemove.RemovePig();
                        pigInfos.Remove(toRemove);
                    }
                }
            });

            refreshSub = refreshTicker.SkipWhile(u => !_refreshEnabled).Subscribe(u =>
            {
                foreach (var pig in pigInfos)
                {
                    if (session != null)
                    {
                        pig.Refresh();
                    }
                }
            });
            fixCrashedPigSrv = refreshTicker.ObserveOnDispatcher(System.Windows.Threading.DispatcherPriority.Normal).Subscribe(u =>
            {
                if (pigSrv == null && session != null)
                {
                    pigSrv = System.Diagnostics.Process.GetProcessById((int)session.ProcessID);
                }
                if (pigSrv != null && pigSrv.HasExited)
                {
                    log("PigSrv Exited!");
                    session = null;
                    pigSrv = null;
                    PigSrvLatestProcessId = 0;
                    var scripts = pigInfos.Select(x => x.ScriptName).ToList();
                    pigInfos.Clear();
                    Connect_Click(null, null);
                    if (RestorePigsEnabled)
                    {
                        log($"Restoring {scripts.Count} Pigs");
                        foreach (var script in scripts)
                        {
                            pigScript = script;
                            CreatePig_Click(null, null);
                        }
                    }
                }
            });
        }

        public event PropertyChangedEventHandler PropertyChanged;

        string _logText = "";
        public string logText
        {
            get { return _logText; }
            set
            {
                _logText = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("logText"));
            }
        }

        private int _minPigs = 4;

        public int MinPigs
        {
            get { return _minPigs; }
            set
            {
                _minPigs = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("MinPigs"));
            }
        }
        private float _minRatio = 2.0f;

        public float MinRatio
        {
            get { return _minRatio; }
            set
            {
                _minRatio = (float)Math.Round(value, 1);
                if (_minRatio + 0.09f > _maxRatio)
                {
                    MaxRatio = _minRatio + 0.1f;
                }
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("MinRatio"));
            }
        }
        private float _maxRatio = 3.0f;

        public float MaxRatio
        {
            get { return _maxRatio; }
            set
            {

                _maxRatio = (float)Math.Round(value,1);
                if (_minRatio > _maxRatio - 0.09f)
                {
                    MinRatio = _maxRatio - 0.1f;
                }
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("MaxRatio"));
            }
        }


        bool _pigButtonEnable = false;
        public bool pigButtonEnable
        {
            get { return _pigButtonEnable; }
            set
            {
                _pigButtonEnable = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("pigButtonEnable"));
            }
        }

        string _pigSript = "";
        public string pigScript
        {
            get { return _pigSript; }
            set
            {
                _pigSript = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("pigScript"));
            }
        }

        List<string> _pigSripts = new List<string>();
        public List<string> pigScripts
        {
            get { return _pigSripts; }
            set
            {
                _pigSripts = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("pigScripts"));
            }
        }

        ObservableCollection<PigInfo> _pigInfos = new ObservableCollection<PigInfo>();
        private bool _restorePigsEnabled = true;

        public ObservableCollection<PigInfo> pigInfos
        {
            get { return _pigInfos; }
            set
            {
                _pigInfos = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("pigInfos"));
            }
        }

        public bool RefreshEnabled
        {
            get => _refreshEnabled; set
            {
                _refreshEnabled = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("RefreshEnabled"));
            }
        }

        public bool RestorePigsEnabled
        {
            get => _restorePigsEnabled; set
            {
                _restorePigsEnabled = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("RestorePigsEnabled"));
            }
        }

        void log(string text)
        {
            logText += string.Format("{0}: {1}\n", DateTime.UtcNow, text);
        }

        private void EnableEvents_Click(object sender, RoutedEventArgs e)
        {
            if (session != null)
            {
                session.ActivateAllEvents();
                log(String.Format("Activated all events"));
            }
        }
        private void DisableEvents_Click(object sender, RoutedEventArgs e)
        {
            if (session != null)
            {
                session.DeactivateAllEvents();
                log(String.Format("Deactivated all events."));
            }
        }

        private void Disconnect_Click(object sender, RoutedEventArgs e)
        {
            if (session != null)
            {
                session = null;
                PigSrvLatestProcessId = 0;
                logText = "";
            }
            pigInfos.Clear();
        }
        private void ClearLog_Click(object sender, RoutedEventArgs e)
        {
            logText = "";
        }

        private void Connect_Click(object sender, RoutedEventArgs e)
        {
            if (session == null)
            {
                log(String.Format("Connecting to PigSrv..."));
                session = new PigsLib.PigSession();
                if (session != null)
                {
                    pigButtonEnable = true;
                    log(String.Format("Connected to process: {0}", session.ProcessID));
                    session.OnEvent += Session_OnEvent;

                    log(String.Format("MaxPigs: {0}", session.MaxPigs));
                    log(String.Format("MissionServer: {0}", session.MissionServer));
                    log(String.Format("AccountServer: {0}", session.AccountServer));
                    //log(String.Format("LobbyMode: {0}", session.LobbyMode));
                    log(String.Format("ZoneAuthServer: {0}", session.ZoneAuthServer));
                    // log(String.Format("ZoneAuthTimeout: {0}", session.ZoneAuthTimeout));
                    log(String.Format("Art Path: {0}", session.ArtPath));
                    log(String.Format("Script Path {0}", session.ScriptDir));

                    // load all the scripts from scriptDir
                    var di = new DirectoryInfo(session.ScriptDir);
                    var newScripts = new List<string>();
                    foreach (var file in di.EnumerateFiles())
                    {
                        if (file.Extension.ToLower().EndsWith(".js"))
                            continue;

                        var justTheName = file.Name.Replace(".pig", "");
                        newScripts.Add(justTheName);
                    }
                    pigScripts = newScripts;
                    if (pigScripts.Count > 0)
                    {
                        pigScript = newScripts.First();
                    }
                    pigInfos.Clear();
                }
            }
        }

        private void CreatePig_Click(object sender, RoutedEventArgs e)
        {
            pigButtonEnable = false;
            if (session != null)
            {
                try
                {
                    log(String.Format("Requesting new Pig.."));
                    var pig = session.CreatePig(pigScript);
                    log(String.Format("Request returned: {0}", pig));
                    if (pig != null)
                    {
                        var pi = new PigInfo(pig, pigScript);
                        pigInfos.Add(pi);
                        pi.OnLogOff += Pi_OnLogOff;
                        log(String.Format("pig {0} : State {1}", pig.Name, pig.PigStateName));
                    }
                }
                catch (Exception ex)
                {
                    log(String.Format("Error: {0}", ex));
                }
            }
            pigButtonEnable = true;
        }

        private void Pi_OnLogOff(PigInfo pig)
        {
            pig.OnLogOff -= Pi_OnLogOff;
            pigInfos.Remove(pig);
        }

        private void Session_OnEvent(AGCLib.IAGCEvent pEvent)
        {
            log(String.Format("Event {0}", pEvent));
        }
    }
}
