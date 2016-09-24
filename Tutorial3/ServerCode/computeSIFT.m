function computeSIFT(input_img_path, output_img_path)
%-----------------------------------------------------------------------------
% EE368 Digital Image Processing
% Android Tutorial #3: Server-Client Communication
% Author: Derek Pang (dcypang@stanford.edu), David Chen (dmchen@stanford.edu)
%------------------------------------------------------------------------------
% INPUT:
% input_img_path 	- input image path
% output_img_path 	- output image path
%------------------------------------------------------------------------------
tic;
%Add VLFeat library for computing SIFT feature
addpath(genpath('./vlfeat-0.9.14/'))

if nargin < 2
    input_img_path =('./upload/test.jpg');
    output_img_path =('./output/test.jpg');
end

if(isempty(input_img_path))
    input_img_path =('./upload/test.jpg');
end
    
if(isempty(output_img_path))
    output_img_path =('./output/test.jpg');
end

imwrite(zeros(640,480), output_img_path,'Quality',100);

% --------------------------------------------------------------------
%                                                        Load an image
% --------------------------------------------------------------------
InputImg = imread(input_img_path) ;
% --------------------------------------------------------------------
%                                       Convert the to required format
% --------------------------------------------------------------------
InputImg = imresize(InputImg,min(1,640/size(InputImg,2)));
GrayImg = single(rgb2gray(InputImg)) ;
% --------------------------------------------------------------------
%                                                             Run SIFT
% --------------------------------------------------------------------
[f,d] = vl_sift(GrayImg) ;


%Construct output image 

h  = figure(1);
%set(h, 'Position',[0 0 1920 1080]);
subplot('Position', [0 0 1 1]) 
image(InputImg);
box off;
grid off;
axis off;

%plot Top 300 SIFT features with largest scale
scale = f(3,:);
[scale_sort, sort_idx] = sort(scale, 'descend');

if numel(sort_idx) > 300
	sel = sort_idx(1:300);
else
   sel = sort_idx;
end

h1   = vl_plotframe(f(:,sel)) ; set(h1,'color','k','linewidth',1.75) ;
h2   = vl_plotframe(f(:,sel)) ; set(h2,'color','y','linewidth',1) ;

truesize(h, [size(InputImg,1) size(InputImg,2)]); %adjust figure
print(h,'-painters','-dbmp16m','./output/temp.bmp')

%convert figure to correct image perspective ratio
outI = imread('./output/temp.bmp') ;
outI = imresize(outI, [size(InputImg,1) size(InputImg,2)]);
imwrite(outI, output_img_path,'Quality',100);
toc
end
