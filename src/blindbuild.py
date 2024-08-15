import subprocess
import os

# Replace 'your_build_target' with the actual build target name
build_target_env = "Unified_ESP8285_2400_RX_via_UART"

def build(env_target, dirpath = "."):
    env = os.environ.copy()
    env["PYTHONUNBUFFERED"] = "1"
    command = ["pio", "run", "-e", env_target, "-d", os.path.abspath(dirpath)]
    process = None
    stdout_output = ""
    stderr_output = ""
    success = False
    text = ""
    try:
        process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
        stdout_output, stderr_output = process.communicate(input='\r', timeout=60*5)
        if process.returncode == 0:
            success = True
            text += "Build Success\n"
        else:
            text += f"Build Failed, code {process.returncode}\n"
        text += stdout_output + '\n' + stderr_output
    except subprocess.TimeoutExpired as e:
        if process is not None:
            process.kill()
            stdout_output, stderr_output = process.communicate()
        else:
            stdout_output = e.stdout
            stderr_output = e.stderr
        text += "ERROR: PIO build process timed out\n" + stdout_output + '\n' + stderr_output
    except Exception as ex:
        text += "ERROR: exception occured during PIO build\n" + str(ex)
    fwpath = os.path.abspath(os.path.join(dirpath, f".pio/build/{env_target}/firmware.bin"))
    if os.path.exists(fwpath) == False:
        text += f"ERROR: \"{fwpath}\" is missing"
        fwpath = None
    return success, text, fwpath

success, txt, fwpath = build(build_target_env)
print(txt)
print(fwpath)
