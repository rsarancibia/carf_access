using Serilog;
using Serilog.Core;
using Serilog.Events;

namespace Edv__Id_Tag_Access.myLog
{
    public static class LogBridge
    {
        public static event Action<LogEvent>? OnLog;

        internal static void Raise(LogEvent logEvent)
        {
            OnLog?.Invoke(logEvent);
        }
    }


    public class InternalSink : ILogEventSink
    {
        public void Emit(LogEvent logEvent)
        {
            LogBridge.Raise(logEvent); // 👈 aquí va
        }
    }

    public static class LoggerConfig
    {
        public static void Init()
        {
            Log.Logger = new LoggerConfiguration()
                .MinimumLevel.Debug()
                .WriteTo.File("dll_log.txt")      // interno
                .WriteTo.Sink(new InternalSink()) // 👈 puente hacia afuera
                .CreateLogger();
        }
    }



}
