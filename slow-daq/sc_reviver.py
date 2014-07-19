from subprocess import check_output, call

output = check_output(['ps', '-u', 'newg2']).split('\n')

ps = []
for proc in output:
 
    if len(proc.split()) != 0:
        ps.append(proc.split()[-1])

start_script = '/home/newg2/Workspace/slac-test-ii/'
start_script += 'daq/slow-daq/launch_slow_daq.sh'

if ('sc_monitor' or 'sc_worker') not in ps:
    
    call(['bash', start_script])
