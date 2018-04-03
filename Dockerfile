FROM segaleran/ceres

RUN cd /opt && \
  wget https://github.com/schuhschuh/gflags/archive/master.zip && \
  unzip master.zip && \
  cd /opt/gflags-master && \
  mkdir build && \
  cd /opt/gflags-master/build && \
  export CXXFLAGS="-fPIC" && \
  cmake .. && \
  make VERBOSE=1 && \
  make && \
  make install

RUN apt-get update && apt-get install -y libopencv-dev

RUN ldconfig

COPY image.jpg /tmp/image.jpg

RUN git clone https://github.com/jackdreilly/ekg.git && \
  cd ekg && \
  mkdir release && \
  cd release && \
  cmake .. && \
  make && \
  mkdir ~/bin && \
  cp ekg ~/bin && \
  export PATH=~/bin:$PATH
