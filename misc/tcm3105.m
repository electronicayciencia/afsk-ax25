%% Software simulation of TCM3105 decoding proccess

clear

%% Input
[A, SR] = audioread('snd/SABM.wav'); % wav file
%[A, SR] = audioread('snd/cal-1200-2200.wav'); % wav file


% Omitted stages: filter, amplifier, AGC and group delay equalizer
x = 1:length(A);

%% Limiter
B = A;
B(B>0) = 1;
B(B<0) = 0;

B = B * 5; % 5V logic

%% Dual-triggered multivibrator
C = B * 0;  % same size as B, default off (zero)

on_time = 200e-6; % 1700 is the mean tone
v_on  = 2.5 + 0.3;
v_off = 2.5 - 0.3;

counter = 0;
b_old = 0;
for i = 1:length(C)
    b = B(i);
    
    if (counter > 0)
        C(i) = 1;
        counter = counter - 1;
    end
    
    % raising edge trigger
    if (b > v_on) && (b_old < v_off)
        counter = on_time * SR;
    end
    
    % falling edge trigger
    if (b < v_off) && (b_old > v_on)
        counter = on_time * SR;
    end
    
    b_old = b;
end

clear b b_old counter on_time i v_on v_off

C = C * 5; % 5V logic

%% Low pass filter
% Obtain DC component from C, using a IIR simple filter
D = C * 0; % allocate D, same size as C

feedback = 0.85; %0.85
d = 0;

for i = 1:length(D)
    d = d*feedback + (1-feedback) * C(i);
    D(i) = d;
end

clear d feedback i

%% Slicer
% let M be -1 and S be 1.

v_space = 4.5; % limit voltage for 2200Hz
v_mark  = 2.0; % limit voltage for 1200Hz

E = D * 0; % not logical, just tones Mark or Space

e = -1;
for i = 1:length(E)
    
    if (e == -1 && D(i) >= v_space) 
        e = 1;
    end
    
    if (e == 1 && D(i) <= v_mark)
        e = -1;
    end
    
    E(i) = e;

end


clear v_mark v_space i e

%% Plot
%plot(A)
%hold on
%%plot(C)
%plot(n,D,n,E)
%plot(E)

%grid on

% End of TCM3105

%% AX25 decoder

% AX.25 is NRZI encoded. 
% Transition is 0, non-transition is 1
% Initial state is 1.


% Supose NRZL (level) encoded
smps = (1/1200) * SR; % samples per symbol
NRZI_DATA = 1;
d_old = 0;
last_tran = 0;
DUR = E * 0;
for i = 1:length(E)
    d = E(i);
    
    % Transition means 0
    if (d ~= d_old)
        % How many ones until this 0?
        n = round((i - last_tran)/smps);
        NRZI_DATA = [NRZI_DATA, ones(1,n-1), 0];
        
        % Desviación al entero más cercano
        n = (i - last_tran)/smps;
        des = abs(n - round(n))/n;
        DUR(i) = n;
        last_tran = i;
    end
    
    d_old = d;
end

clear d d_old i smps last_tran n 

%hold off
%plot(E)
%hold on
%plot(NRZI_TIME)
%plot(DESV)
%grid on

%%

% Print string
toStringJSON(NRZI_DATA)

% Next steps:
%  - Keep bits between 0111110 ... 0111110 flags
%  - Remove bit stuffing s/111110/11111/g
%  - Group octets
%  - Flip bits (AX25 is LSB first)
%  - Decode packet

%%
plot (x,A)
xlabel('nº de muestra (44100Hz)');
ylabel('Valor');
title('Detalle señal FSK');
grid

%%
plot (x,B,x,A)
xlabel('nº de muestra (44100Hz)');
ylabel('Valor');
title('Señal FSK cuadrada');
grid

%%
plot (x,C,x,A)
xlabel('nº de muestra (44100Hz)');
ylabel('Valor');
title('Efecto del flip flop de duración fija');
grid

%%
c = square(2*pi*600*(x-10)/SR)*3+2;
plot (x,D,x,A,x,4.5*ones(size(x)),x,2*ones(size(x)),x,c)

xlabel('nº de muestra (44100Hz)');
ylabel('Valor');
title('Efecto del filtro paso bajo');
grid

%%
plot (x,E,x,A)
xlabel('nº de muestra (44100Hz)');
ylabel('Valor');
title('Efecto del "slicer"');
grid

%%
plot (x,DUR)
xlabel('nº de muestra (44100Hz)');
ylabel('Duración en símbolos');
title('Decodificación NZRI');
grid


