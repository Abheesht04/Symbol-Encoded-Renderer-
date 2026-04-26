# Symbol-Encoded-Renderer-

After downloading the Zip file run this command in the vision folder of your windows powershell,
gcc -o build_pipeline.exe main.c deflate.c png_reader.c png_process.c symbolizer.c geometry.c ../core/rle.c ../core/huffman.c ../core/huffman_canonical.c ../core/decode_huffman.c ../core/encoder.c ../core/decoder.c ../core/bitstream.c -I. -I../core

After that run the exe using this command - ./build_pipeline.exe


Updated the new Microsoft Visual studio 2019 and a sample render of a small minecraft like scene has been achieved 
