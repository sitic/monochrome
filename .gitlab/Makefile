build:
	docker build --pull -t registry.gitlab.com/cardiac-vision/monochrome:latest .
	docker build -t registry.gitlab.com/cardiac-vision/monochrome/gcc:latest -f Dockerfile.gcc .
	docker build -t registry.gitlab.com/cardiac-vision/monochrome/clang:latest -f Dockerfile.clang .

push:
	docker push registry.gitlab.com/cardiac-vision/monochrome
	docker push registry.gitlab.com/cardiac-vision/monochrome/gcc
	docker push registry.gitlab.com/cardiac-vision/monochrome/clang

clean:
	docker rmi registry.gitlab.com/cardiac-vision/monochrome/gcc docker.gitlab.com/cardiac-vision/monochrome/clang docker.gitlab.com/cardiac-vision/monochrome
