from fabric.api import run

def deploy():
    run("service rsp stop")
    run("cd /home/rsp/rsp/src && git pull origin master && make clean && make && make install")
    run("service rsp start")
