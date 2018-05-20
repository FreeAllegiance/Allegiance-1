using System;
using System.Windows.Input;
using PigsLib;

namespace PigClient
{

    public class CommandWrapper : ICommand
    {
        private Action _execute;
        private Func<bool> _canExecute;

        public CommandWrapper(Action execute, Func<bool> canExecute)
        {
            _execute = execute;
            _canExecute = canExecute;
        }

        public event EventHandler CanExecuteChanged;

        public bool CanExecute(object parameter)
        {
            return _canExecute();
        }

        public void Execute(object parameter)
        {
            _execute();
        }
    }

}
