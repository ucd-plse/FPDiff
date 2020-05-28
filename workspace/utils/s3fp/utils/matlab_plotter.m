function matlab_plotter(inname, time_plot_name, elog_plot_name, max_distance)
tdata = load(inname);


% ---- for time_plot ---- 
esum = 0;
e2sum = 0;
for i=1:length(tdata),
  fperr(i) = tdata(i,:);
  esum = esum + fperr(i);
  e2sum = e2sum + (fperr(i) * fperr(i));
end;
nerrs = length(tdata);
eave = esum / nerrs; 
evar = e2sum - (eave * eave);
gap = evar; 


% ---- for inv_plot ---- 
for i=1:((max_distance*2)+2),
  if i <= (max_distance + 1),
    errx(i) = i - (max_distance + 2);
  else,
    errx(i) = i - (max_distance + 1); 
  end;
  errdist(i) = 0;
end;

for i=1:length(fperr), 
  if fperr(i) >= eave, 
    distance = ceil((fperr(i) - eave) / gap);
    if distance > max_distance,
      distance = max_distance + 1;
    end; 
    distance = distance + (max_distance + 1);
    errdist(distance) = errdist(distance) + 1;
  else, 
    distance = ceil((eave - fperr(i)) / gap);
    if distance > max_distance, 
      distance = max_distance + 1;
    end;
    distance = (max_distance + 2) - distance; 
    errdist(distance) = errdist(distance) + 1;
  end; 
end;


% ---- export time_plot ----
figure(1);
plot(1:length(tdata), fperr, '-'); 
saveas(figure(1), time_plot_name);


% ---- export byave_plot ---- 
figure(2);
semilogy(1:length(tdata), fperr, '-');
saveas(figure(2), elog_plot_name);


% ---- export inv_plot ---- 
% figure(3);
% bar(errx, errdist); 
% saveas(figure(3), inv_plot_name);
  
