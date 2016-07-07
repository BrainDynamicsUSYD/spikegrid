function showdynamics(varGrid,varargin)

% Displays voltage and conductance data. When the job these two figures will pop up:
% Figure 1: shows values across the grid at one time step.
% Figure 2: shows values across time at one grid point, with dashed horizontal line indicating the current time
% There will also be a MENU which has the following options:
% > : Move forward a small amount in time (default is +1 time step)
% >> : Move forward a large amount in time (default is +10 time steps)
% < : Move backward a small amount in time (default is -1 time steps)
% < : Move backward a large amount in time (default is -10 time steps)
% time: Skip to a specified time. A prompt will appear in the terminal asking you to type in a desired time
% trace: Pick a new neuron to obtain the trace of (Figure 2). A target will appear when you move the cursor over Figure 1. Clicking on a particular neuron will update the trace to show that neuron's dynamics
% fig: Writes a matlab figure and a png file of Figure 1 to the current directory. A prompt will ask you to type a name for the file into the terminal. Make sure you use a ' before and after the name so MATLAB knows it's a string e.g. 'myfilename'
% end: Closes this function but retains the figures

% INPUT
% varGrid data, which can be obtained by using the txt2mat.m function to extract data from the C simulation. For a simulation of grid NxN with T recorded time intervals, varGrid is a matrix of dimensions N^2 x T
% varargin (optional): Set minumum and maximum values for colormap and y-axis of the trace, like this: [max,min], e.g. [0,1] for minimum of 0 and maximum of 1. If unspecified the code detects the smallest and largest values in the entire data set and uses them


% Close lingering figures
close all

% Length of grid
N = sqrt(size(varGrid,1));
if mod(N,1)~=0
	error('Grid must be square, i.e. square root of the length of the first dimension of vargrid (phew!) must be an integer')
end

% Length of time series
T = size(varGrid,2); 

% Make into NxNxT grid
varGrid3D = reshape(varGrid,[N,N,T]);  

% Optional argument
if length(varargin)==0
	minval = min(varGrid3D(:));
	maxval = max(varGrid3D(:));
	maxmin = [minval,maxval];
elseif length(varargin)==1
	maxmin = varargin{1};
else
	error('Wrong number of input arguments! Must be 2 or 3 arguments')
end

% Setup colormap
fig = figure(1);
% colormap(jet) % If you want to old colormap (which can contain artefacts)
fig_V = imagesc(varGrid3D(:,:,1),maxmin);
fig_t = title('Time: 1');

% Setup trace
figure(2)
hold on
tr_V = plot(1:T,squeeze(varGrid3D(1,1,:)),'g.-');
tr_t = plot([1,1],[maxmin(1),maxmin(2)],'k--');
tr_n = title('Neuron at (1,1)')
hold off
xlabel('Time');
ylim(maxmin);

% Set up menu
smallstep = 1;
bigstep = 10;
t = 1; % Initialize counter
while 1
	invalid = true;
	while invalid
		choice = menu('Time step','>','>>','<','<<','time','trace','fig','end');
		% Small step forward
		if choice==1
			t_=t+smallstep;
		% Large step forward
		elseif choice==2
			t_=t+bigstep;
		% Small step back
		elseif choice==3
			t_=t-smallstep;
		% Large step back
		elseif choice==4
			t_=t-bigstep;
		% Input desired time into terminal
		elseif choice==5
			t_=input('Enter time to skip to: ');
		% Extract trace for one neuron
		elseif choice==6
			[y,x] = ginput(1);
			x = ceil(x);
			y = ceil(y);
			tr_V.YData = squeeze(varGrid3D(x,y,:));
			tr_n.String = sprintf('Neuron at (%d,%d)',x,y);
		% Save current figure as png+fig
		elseif choice==7
			htemp = figure;
			figname = input('Figure name (put in quotes): ')
			imagesc(varGrid3D(:,:,t),maxmin);
			title(sprintf('Time: %.2f ms',t))
			nicefig;
			saveas(gcf,figname,'fig');
			saveas(gcf,figname,'png');
			close(htemp)
		elseif choice==8
			return
		end
		if t_>T 
			warning('You are trying to take a step which exceeds the maximum duration of this simulation; try another option')
			invalid = true;
		elseif t_<=0
			warning('You are attempting to access a negative time step; try another option')
			invalid = true;
		else
			t=t_;
			invalid = false;
		end
	end

	% Update figure handles
	fig_V.CData = varGrid3D(:,:,t);
	fig_t.String = sprintf('Time: %d',t);
	tr_t.XData = [t,t];
end

end


function nicefig

hFig = gcf;

% Get axis handle and remove box
hAxes = get(hFig,'CurrentAxes');
hAxes.Box = 'Off'
if nargin>0
	if strcmp(varargin{1},'notick');
		hAxes.XTick = [];
		hAxes.YTick = [];
	end
end

% Undock figure (needs to be undocked to change position)
set(hFig,'WindowStyle','normal');

% Set font type and size
FontName = 'Helvetica';
FontSize = 16;

%Set all Font Sizes
set(findall(hFig,'-property','FontSize'),'FontSize',FontSize);
set(findall(hFig,'-property','FontName'),'FontName',FontName);

% Colour background
set(hFig,'Color','w');

% % Remove axes ticks
% hax = gca;
% hax.XTick=[];
% hax.YTick=[];

end

