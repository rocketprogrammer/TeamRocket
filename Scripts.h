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

inline const char *fastDanceAnim = R"(
base.localAvatar.b_setAnimState('victory', 10)
)";

inline const char *neutralAnim = R"(
base.localAvatar.b_setAnimState('neutral', 1)
)";

inline const char *danceAnim = R"(
base.localAvatar.b_setAnimState('victory')
)";

// TODO: Restore original teleport access.
inline const char *globalTeleport = R"(
from toontown.toonbase import ToontownGlobals

base.localAvatar.setTeleportAccess(ToontownGlobals.HoodsForTeleportAll)
base.localAvatar.setHoodsVisited(ToontownGlobals.HoodsForTeleportAll)  
)";