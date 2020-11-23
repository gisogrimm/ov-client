function plot_pinghist( ping )
  %ping(3,:) = 0.5*ping(3,:);
  devlist = unique( ping(2,:) );
  vbins = [0:0.5:60];
  cols = jet(numel(devlist))*0.9;
  wpos = [10,10,1200,600];
  figure;%('position',wpos,'paperunits','centimeters','paperposition',0.04*wpos);
  k = 0;
  csLabels = {};
  for dev=devlist
    csLabels{end+1} = sprintf('dev%d',dev);
  end
  vobj = [];
  vinv = [];
  for dev=devlist
    k = k+1;
    idx = find(ping(2,:)==dev);
    h = hist(ping(3,idx),vbins);
    if( sum(h) > 0 ) 
      h = h/sum(h);
    end
    h(find(h==0)) = inf;
    hp = plot(vbins,h,'linewidth',4,'Color',cols(k,:));
    hold on;
    vt = quantile(ping(3,idx),[0,0.99]);
    dy = 0.22;
    py = 1-dy*(k-1)-dy*[0,1,1,0];
    marg = 0.01;
    vp = patch(vt([1,1,2,2]),py-marg*[1,-1,-1,1],0.7+0.3*cols(k,: ...
						  ));
    vobj(end+1) = vp;
    lossp = numel(find(ping(3,idx)>vt(1)+10))/numel(idx)*100;
    set(vp,'FaceColor', 0.7+0.3*cols(k,:));
    vobj(end+1) = text(vt(1),py(1)-0.5*dy,...
		       sprintf('- %s:\n  network %1.1f ms\n  jitter (1%% loss) %1.1f ms\n  loss (10ms buffer) %1.1f %%',csLabels{k},vt(1),diff(vt),lossp),...
		       'FontSize',14,'HorizontalAlignment','left','FontWeight', ...
		       'bold','Color',0.8*cols(k,:));
    vinv(end+1) = text(vt(1),py(1)-0.5*dy,sprintf('- %s\n \n \n',csLabels{k}),...
		       'FontSize',14,'HorizontalAlignment','left','FontWeight', ...
		       'bold','Color',0.8*cols(k,:),'visible','off');
    vobj(end+1) = plot(vt([1,1])+10,py([1,2])-marg*[1,-1],'-','Color',0.*cols(k,:),'linewidth',0.5);
  end
  plot([min(vbins),max(vbins)],[0,0],'k-','linewidth',1);
  for( tt=[20:10:50] )
    x = (tt-20)*2;
    %plot([x,x],[1,1.02],'k-');
    text(x,1.04,num2str(tt),'fontsize',16,'HorizontalAlignment','center');
  end
  text(0.5*max(vbins),1.08,'peer latency in ms','fontsize',16,'HorizontalAlignment','center');
  ylim([-0.001,1]);
  xlim([min(vbins),max(vbins)]);
  
  set(gca,'FontSize',16,'Fontname','Nunito Sans light');
  xlabel('ping time in ms');
  ylabel('rel. frequency of occurrence');
  saveas(gcf,'pingtime.png');
  %system('inkscape slides/pingtime.eps -l slides/pingtime.svg');
  %delete(vobj);
  %set(vinv,'visible','on');
  %saveas(gcf,'slides/pingtime1.eps','epsc');
  %system('inkscape slides/pingtime1.eps -l slides/pingtime1.svg');
