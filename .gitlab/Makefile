build:
	docker build --pull -t docker.gitlab.gwdg.de/jlebert/quickmultrecviewer:latest .
	docker build -t docker.gitlab.gwdg.de/jlebert/quickmultrecviewer/gcc:latest -f Dockerfile.gcc .
	docker build -t docker.gitlab.gwdg.de/jlebert/quickmultrecviewer/clang:latest -f Dockerfile.clang .

push:
	docker push docker.gitlab.gwdg.de/jlebert/quickmultrecviewer
	docker push docker.gitlab.gwdg.de/jlebert/quickmultrecviewer/gcc
	docker push docker.gitlab.gwdg.de/jlebert/quickmultrecviewer/clang

clean:
	docker rmi docker.gitlab.gwdg.de/jlebert/quickmultrecviewer/gcc docker.gitlab.gwdg.de/jlebert/quickmultrecviewer/clang docker.gitlab.gwdg.de/jlebert/quickmultrecviewer
