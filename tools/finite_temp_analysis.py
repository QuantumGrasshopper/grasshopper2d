# data analysis routines for fixed finite temperature simulations

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import os
import glob
from scipy.stats import moment
from scipy.optimize import curve_fit, minimize
from scipy.interpolate import make_interp_spline

# function definitions ------------------------------------------------------

# make a list of all sub-directories containing result files
def find_result_files(root_dir):
    result_dirs = []

    for file_path in glob.glob(os.path.join(root_dir, '**', 'result.dat'), recursive=True):
        current_dir = os.path.basename(os.path.dirname(file_path))
        result_dirs.append(current_dir)
    
    return result_dirs

def yes_no_question(answer):
    if answer.lower() in ["y", "yes", "1"]:
        return True
    else:
        return False

def fermi_fn(x,width):
    return 1/(1+np.exp((x-r0)/width))

# read raw boundary data, shifts + scales it, returns interpolating spline
def process_boundary(shift, scale):
    
    # adjust this if boundary data is elsewhere
    polar_coordinates = np.loadtxt(f'{root_dir}/boundary.txt')
    polar_coordinates[:,0] += shift
    polar_coordinates[polar_coordinates[:,0] > np.pi,0] -= 2*np.pi
    polar_coordinates = polar_coordinates[polar_coordinates[:, 0].argsort()]
    polar_coordinates[:,1] = (polar_coordinates[:,1]-r0)*scale+r0
    
    bspl = make_interp_spline(polar_coordinates[:,0],polar_coordinates[:,1]/r0, k=1)
    
    return bspl

def fitted_width(params):
    
    bspl = process_boundary(params[0], params[1])
    hist, bin_edges = np.histogram(rho/bspl(phi), bins=60, density=True)
    rho_values = (bin_edges[:-1]+bin_edges[1:])/2

    popt, pcov = curve_fit(fermi_fn, rho_values, hist/rho_values/2/np.pi)
    return popt[0]

# loop through all subdirectories -------------------------------------------

root_dir = "/home/olga/Documents/grasshopper/grasshopper2d/runs/fixedTemps/d0.27N100k/test/"
result_directories = find_result_files(root_dir)

output_file = 'results_table.txt'
output_file_path = os.path.join(root_dir, output_file)

header_format = f"# {'jump':<5}\t{'N':<6}\t{'T':<4}\t{'L':<4}\t{'cutoff':<6}\t{'mean(E)':<12}\t{'var(E)':<12}\t{'mom4(E)':<12}\t{'width':<10}\t{'scale':<10}\n"
with open(output_file_path, 'w') as f:
    f.write(header_format)

for current_dir in result_directories:
    
    print("Processing data in ", current_dir)

    # read in data --------------------------------------------------------------
    
    with open(f"{root_dir}/{current_dir}/result.dat") as f:
        lines = f.readlines()
        jump = float(lines[3].strip().split(" ")[-1])
        gridsize = int(lines[5].strip().split(" ")[-1])
        temperature = float(lines[10].strip().split(" ")[-1])
        
    finconf = np.loadtxt(f"{root_dir}/{current_dir}/finconf.dat", dtype = int)
    numberspins = len(finconf)
    cellwidth = numberspins**(-1/2)
    
    energy = np.loadtxt(f"{root_dir}/{current_dir}/energies.dat", dtype = float)
    
    # find energy cutoff --------------------------------------------------------------
    
    cutoff = len(energy)//10
        
    energymean = np.mean(energy[-cutoff:])
    energystd = np.std(energy[-cutoff:])
    
    for i in range(len(energy)):
        if abs(energy[i]-energymean)<energystd:
            cutoff=i
            break
    
    # prompt to manually examine and maybe adjust cutoff
    cutoff_approved = False;
    while cutoff_approved == False:
        
        # show energy with cutoff
        plt.plot(energy)
        plt.axvline(cutoff)
        plt.show()
        
        # zoom in on the energy plot when system is thermalized
        plt.plot(energy[cutoff:])
        plt.axhline(energymean, color = 'k')
        plt.axhline(energymean+energystd, color = 'k')
        plt.axhline(energymean-energystd, color = 'k')
        plt.show()
        
        print("The current cutoff is ", cutoff)
        cutoff_approved = yes_no_question(input("Is the current cutoff sufficient? (y/n) "))
        if cutoff_approved == False:
            cutoff += int(input("Please enter how many additional points to add to cutoff: "))
    
    
    energymean = np.mean(energy[cutoff:])
    energystd = np.std(energy[cutoff:])
    energyvar = np.var(energy[cutoff:])
    energymom4 = moment(energy[cutoff:], 4)
    
    # check if energy distribution looks Gaussian
    x = np.linspace(energymean-3*energystd,energymean+3*energystd,200)
    y = np.exp(-(x-energymean)**2/2/energyvar)/np.sqrt(2*np.pi*energyvar)
    
    plt.hist(energy[cutoff:], density=True, bins=40)
    plt.plot(x, y)
    plt.show()
    
    # plot final configuration --------------------------------------------------------------
    
    unraveled = np.unravel_index(finconf, (gridsize, gridsize))
    grid = np.zeros((gridsize, gridsize), dtype=int)
    grid[unraveled] = 1
    
    greens = plt.cm.Greens(np.linspace(0, 1, 256))
    colors = np.vstack([[1, 1, 1, 1], greens[1:]])  # Start with white and then use Greens
    custom_greens = LinearSegmentedColormap.from_list('CustomGreens', colors)
    
    plt.imshow(grid, interpolation='None', cmap=custom_greens, origin = 'lower')
    plt.axis('off')
    fig_name = f"d{jump:.2f}_N{numberspins}_T{temperature}_finconf.pdf"
    plt.savefig( os.path.join(root_dir, fig_name) )
    plt.show()
    plt.clf()
    
    # configuration center of mass --------------------------------------------------------------
    
    com=np.zeros(2)
    
    for i in range(gridsize):
        for j in range(gridsize):
            com[0]+=grid[i,j]*i
            com[1]+=grid[i,j]*j
                
    com = com/np.sum(grid)
    
    # configuration polar representation --------------------------------------------------------------
    
    phi = []
    rho = []
    r0 = np.sqrt(1/np.pi)   # radius of the unit area disc
    
    for i in range(gridsize):
        for j in range(gridsize):
            if grid[i,j] == 1:
                x = (i - com[0])*cellwidth
                y = (j - com[1])*cellwidth
                phi.append(np.arctan2(y,x))
                rho.append(np.sqrt(x*x+y*y))
    
    # plot of rho vs phi
    plt.xlim(-np.pi,np.pi)
    plt.ylim(0,gridsize/2*cellwidth)
    plt.xlabel(r'$\phi$', fontsize = 16)
    plt.ylabel(r'$\rho$', fontsize = 16)
    plt.scatter(phi, rho, s=0.05)
    plt.axhline(r0, color='k')
    
    fig_name = f"d{jump:.2f}_N{numberspins}_T{temperature}_polar.pdf"
    plt.savefig( os.path.join(root_dir, fig_name) )
    plt.clf()
        
    # histogram of radii, integrated over phi
    bins=60
    plt.hist(rho, bins=bins, density=True)
    plt.axvline(r0, color='k')
    hor = np.linspace(0,r0,100)
    plt.plot(hor, 2*np.pi*hor, color='k')
    plt.xlabel(r'$\rho$', fontsize = 16)
    
    fig_name = f"d{jump:.2f}_N{numberspins}_T{temperature}_hist.pdf"
    plt.savefig( os.path.join(root_dir, fig_name) )
    plt.clf()
    
    # check if systematic boundary removal needed --------------------------------------------------------------
    
    scale = 0
    bound_removal = yes_no_question(input("Are there still visible cogs that need to be removed? (y/n) "))
    
    if bound_removal == True:
        
        # count number of cogs
        cogs = int(input("Enter number of cogs: "))
        freq = 2*np.pi/cogs
    
        # boundary minimization, start with no shift and no scaling
        x0 = np.array([0, 1])
        bnds = ((0, freq), (0, 1))
        res = minimize(fitted_width, x0, method='powell', bounds = bnds, tol=1e-6)
        shift = res.x[0]
        scale = res.x[1]
        width = fitted_width(res.x)
    
        # show final results
        x_for_plot = np.linspace(-np.pi,np.pi,500)
        bspl = process_boundary(shift, scale)
    
        plt.xlim(-np.pi,np.pi)
        plt.ylim(0,gridsize/2*cellwidth)
        plt.scatter(phi, rho, s=0.05)
        plt.plot(x_for_plot, bspl(x_for_plot)*r0, color = 'k')
        plt.axhline(r0, color='k')
        plt.show()
    
        plt.xlim(-np.pi,np.pi)
        plt.ylim(0,gridsize/2*cellwidth)
        plt.scatter(phi, rho/bspl(phi), s=0.05)
        plt.axhline(r0, color='k')
        plt.show()
    
        plt.hist(rho/bspl(phi), bins=bins, density=True)
        plt.axvline(r0, color='k')
        hor = np.linspace(0,r0,100)
        plt.plot(hor, 2*np.pi*hor, color='k')
        plt.show()
    
        # save boundary width figure
        hist, bin_edges = np.histogram(rho/bspl(phi), bins=bins, density=True)
        rho_values = (bin_edges[:-1]+bin_edges[1:])/2
    
        plt.plot(rho_values, hist/rho_values/2/np.pi)
        plt.plot(rho_values, fermi_fn(rho_values,width))
        plt.xlabel(r'$\rho$', fontsize = 16)
        plt.axvline(r0, color='k')
        plt.axhline(1, color='k')
    
        fig_name = f"d{jump:.2f}_N{numberspins}_T{temperature}_boundary.pdf"
        plt.savefig( os.path.join(root_dir, fig_name) )
        plt.clf()

    else: 
        # fit Fermi function to determine width  ------------------------------
    
        hist, bin_edges = np.histogram(rho, bins=bins, density=True)
        rho_values = (bin_edges[:-1]+bin_edges[1:])/2
    
        popt, pcov = curve_fit(fermi_fn, rho_values, hist/rho_values/2/np.pi)
        width = popt[0]
    
        # save boundary width figure
        plt.plot(rho_values, hist/rho_values/2/np.pi)
        plt.plot(rho_values, fermi_fn(rho_values,popt[0]))
        plt.xlabel(r'$\rho$', fontsize = 16)
        plt.axvline(r0, color='k')
        plt.axhline(1, color='k')
        plt.ylim(0,1.1)
        
        fig_name = f"d{jump:.2f}_N{numberspins}_T{temperature}_boundary.pdf"
        plt.savefig( os.path.join(root_dir, fig_name) )
        plt.clf()

    # print all data to file --------------------------------------------------------------

    all_output = f"{jump:.2f}\t{numberspins}\t{temperature:2.1f}\t{gridsize:4d}\t{cutoff:4d}\t{energymean:.6e}\t{energyvar:.6e}\t{energymom4:.6e}\t{width:.6f}\t{scale:.4f}\n"
        
    with open(output_file_path, 'a') as f:  # Open the file in append mode
        f.write(all_output)
        
        
        
        
