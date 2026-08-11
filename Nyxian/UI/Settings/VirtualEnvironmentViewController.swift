/*
 SPDX-License-Identifier: AGPL-3.0-or-later

 Copyright (C) 2025 - 2026 emexlab

 This file is part of Nyxian.

 Nyxian is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Nyxian is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Nyxian. If not, see <https://www.gnu.org/licenses/>.
*/

import UIKit

class VirtualEnvironmentViewController: UIThemedTableViewController {
    override func viewDidLoad() {
        super.viewDidLoad()
        title = "Virtual Environment"
        view.backgroundColor = .systemBackground
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return 2
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let tableViewCell: UITableViewCell = UITableViewCell()
        
        if indexPath.row == 0 {
            tableViewCell.textLabel?.text = "Userspace Reboot"
        } else if indexPath.row == 1 {
            tableViewCell.textLabel?.text = "Restore"
        }
        return tableViewCell
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        if indexPath.row == 0 {
            PEUserspaceManager.shared().rebootUserspace()
        } else if indexPath.row == 1 {
            let alert = UIAlertController(
                title: "Restore",
                message: "All apps, binaries and data containers in the virtual environment will be wiped.",
                preferredStyle: .alert
            )
            
            alert.addAction(UIAlertAction(title: "Proceed", style: .destructive) { _ in
                DispatchQueue.main.async {
                    let alert = UIAlertController(title: nil, message: "Restoring", preferredStyle: .alert)
                    
                    let activityIndicator = UIActivityIndicatorView(style: .medium)
                    activityIndicator.translatesAutoresizingMaskIntoConstraints = false
                    activityIndicator.startAnimating()
                    
                    alert.view.addSubview(activityIndicator)
                    
                    NSLayoutConstraint.activate([
                        activityIndicator.centerYAnchor.constraint(equalTo: alert.view.centerYAnchor),
                        activityIndicator.trailingAnchor.constraint(equalTo: alert.view.trailingAnchor, constant: -20)
                    ])
                    
                    self.present(alert, animated: true)
                    
                    DispatchQueue.global().async {
                        PEUserspaceManager.shared().restore()
                        DispatchQueue.main.async {
                            alert.dismiss(animated: true)
                        }
                    }
                }
            })
            
            alert.addAction(UIAlertAction(title: "Keep Data", style: .cancel))
            
            self.present(alert, animated: true)
        }
        tableView.deselectRow(at: indexPath, animated: true)
    }
}
