build:
	docker build --pull -t docker.gitlab.gwdg.de/lebert/monochrome:latest .
	docker build -t docker.gitlab.gwdg.de/lebert/monochrome/gcc:latest -f Dockerfile.gcc .
	docker build -t docker.gitlab.gwdg.de/lebert/monochrome/clang:latest -f Dockerfile.clang .

push:
	docker push docker.gitlab.gwdg.de/lebert/monochrome
	docker push docker.gitlab.gwdg.de/lebert/monochrome/gcc
	docker push docker.gitlab.gwdg.de/lebert/monochrome/clang

clean:
	docker rmi docker.gitlab.gwdg.de/lebert/monochrome/gcc docker.gitlab.gwdg.de/lebert/monochrome/clang docker.gitlab.gwdg.de/lebert/monochrome
