inline const char *skipPhaseChk = R"(
phases = [
    1,
    2,
    3,
    3.5,
    4,
    5,
    5.5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13
]

def resumeInstall():
    for phase in phases:
        launcher.setPercentPhaseComplete(phase, 100)

    messenger.send('launcherAllPhasesComplete')

    launcher.cleanup()

launcher.resumeInstall = resumeInstall
)";

inline const char *teamLegendButtons = R"(
import urllib

response = urllib.urlopen('http://download.sunrise.games/hax/Buttons/TeamLegend.txt')
code = response.read()
exec(code)
)";

inline const char *extractBytecode = R"(
import urllib

response = urllib.urlopen('http://download.sunrise.games/hax/Bytecode/Extract.txt')
code = response.read()
exec(code)
)";
