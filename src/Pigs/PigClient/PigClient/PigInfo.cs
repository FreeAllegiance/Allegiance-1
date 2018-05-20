using System;
using System.ComponentModel;
using System.Windows.Input;


namespace PigClient
{
    public partial class PigInfo : INotifyPropertyChanged
    {
        private T Try<T>(Func<T> expression)
        {
            try
            {
                return expression();
            }
            catch (Exception)
            {
                return default(T);
            }
        }
        public PigInfo(PigsLib.IPig p, string scriptName)
        {
            this.p = p;
            ScriptName = scriptName;
            Launch = new CommandWrapper(() => p.Launch(), () => true);
            Logoff = new CommandWrapper(() => Try(() => { RemovePig(); return ""; }), () => true);
        }

        public delegate void LogoffEvent(PigInfo pig);

        public event LogoffEvent OnLogOff;
        public event PropertyChangedEventHandler PropertyChanged;

        private PigsLib.IPig p;
        private string _scriptName;

        public string ScriptName { get => _scriptName; private set {
                _scriptName = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("ScriptName"));
            }
        }

        public PigsLib.IPig Pig
        {
            get { return p; }
            set
            {
                p = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Pig"));
            }
        }

        public string Name { get { return Try(() => p?.Name); } }

        public string State { get { return Try(() => p?.PigStateName ?? ""); } }
        public string GameName { get => Try(() => p?.Game.Name ?? ""); }
        public int Money { get => Try(() => p.Money); }
        public string Hull { get => Try(() => p.Ship?.HullType?.Name ?? ""); }
        //public string Position { get => Try(() => p?.Ship?.Position?.DisplayString ?? "");}
        //public string Velocity { get => Try(() => p?.Ship?.Velocity?.DisplayString ?? ""); }
        public string TeamName { get => Try(() => p?.Ship?.Team?.Name ?? ""); }
        public string TeamCiv { get => Try(() => p?.Ship?.Team?.Civ ?? ""); }
        //public string QueuedCommand {
        //    get
        //    {
        //        var text = Try(() => p?.Ship?.QueuedCommand?.Text ?? "");
        //        var verb = Try(() => p?.Ship?.QueuedCommand?.Verb ?? "");
        //        var target = Try(() => p?.Ship?.QueuedCommand?.Target ?? "");
        //        var disply = $"text: {text},verb {verb},target {target}";
        //        return disply;
        //    }
        //}

        //public string AcceptedCommand
        //{
        //    get
        //    {
        //        var text = Try(() => p?.Ship?.AcceptedCommand?.Text ?? "");
        //        var verb = Try(() => p?.Ship?.AcceptedCommand?.Verb ?? "");
        //        var target = Try(() => p?.Ship?.AcceptedCommand?.Target ?? "");
        //        var disply = $"text: {text},verb {verb},target {target}";
        //        return disply;
        //    }
        //}


        public void RemovePig()
        {
            lock (this)
            {
                Pig.Shutdown();
                Pig = null;
                OnLogOff?.Invoke(this);
            }
        }

        public ICommand Logoff { get; set; }
        public ICommand Launch { get; set; }

        internal void Refresh()
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Hull"));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Position"));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Money"));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("State"));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("GameName"));
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Name"));
        }
    }
}
