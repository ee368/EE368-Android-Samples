% VL_DEMO_SIFT_BASIC  Demo: SIFT: basic functionality

pfx = fullfile(vl_root,'figures','demo') ;
randn('state',0) ;
rand('state',0) ;
h = figure(1) ; clf ;
tic;
% --------------------------------------------------------------------
%                                                        Load a figure
% --------------------------------------------------------------------
I = imread(fullfile(vl_root,'data','a.jpg')) ;
clf ; imagesc(I)


% --------------------------------------------------------------------
%                                       Convert the to required format
% --------------------------------------------------------------------
I = single(rgb2gray(I)) ;



% --------------------------------------------------------------------
%                                                             Run SIFT
% --------------------------------------------------------------------
[f,d] = vl_sift(I) ;

hold on ;
perm = randperm(size(f,2)) ;
sel  = perm(1:100) ;


h3 = vl_plotsiftdescriptor(d(:,sel),f(:,sel)) ;
set(h3,'color','k','linewidth',2) ;
h4 = vl_plotsiftdescriptor(d(:,sel),f(:,sel)) ;
set(h4,'color','g','linewidth',1) ;
h1   = vl_plotframe(f(:,sel)) ; set(h1,'color','k','linewidth',3) ;
h2   = vl_plotframe(f(:,sel)) ; set(h2,'color','y','linewidth',2) ;
toc;

saveas(h,'/var/www/dcypang/ee368/output/test.jpg')
