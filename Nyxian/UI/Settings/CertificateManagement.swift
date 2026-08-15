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

class CertificateController: UITableViewController {
    var certificateStateCell: UITableViewCell?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.title = "Certificate"
        self.tableView.rowHeight = UITableView.automaticDimension
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return 2
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        if indexPath.row == 1 {
            let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
            cell.textLabel?.text = "Set Up Signing"
            cell.textLabel?.textColor = .label
            cell.textLabel?.textAlignment = .left
            cell.selectionStyle = .default
            return cell
        } else {
            let cell: UITableViewCell = UITableViewCell()
            certificateStateCell = cell
            updateCertificateState()
            return cell
        }
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        if indexPath.row == 1 {
            importCertificate()
        }
        tableView.deselectRow(at: indexPath, animated: true)
    }
    
    func importCertificate() {
        let importPopup: CertificateImporter = CertificateImporter(style: .insetGrouped) { [weak self] in
            guard let self = self else { return }
            self.updateCertificateState()
        }
        let importSettings: UINavigationController = UINavigationController(rootViewController: importPopup)
        importSettings.modalPresentationStyle = .formSheet
        
        // dynamic size
        if UIDevice.current.userInterfaceIdiom == .phone {
            if let sheet = importSettings.sheetPresentationController {
                sheet.animateChanges {
                    sheet.detents = [
                        .custom { _ in
                            return 200
                        }
                    ]
                }
                    
                sheet.prefersGrabberVisible = true
            }
        }
        
        self.present(importSettings, animated: true)
    }
    
    private var isAnimatingCertState = false
    func updateCertificateState() {
        assert(Thread.isMainThread)
        isAnimatingCertState = true
        if let certificateStateCell = self.certificateStateCell {
            LCUtils.validateCertificate { [weak self] status, experiationDate, someWords in
                DispatchQueue.main.async {
                    UIView.animate(withDuration: 0.1) {
                        certificateStateCell.textLabel?.alpha = 0.0
                    } completion: { finished in
                        certificateStateCell.textLabel?.textColor = status == 0 ? UIColor.systemGreen : UIColor.systemRed
                        certificateStateCell.textLabel?.text = status == 0 ? "Certificate Valid Till \(experiationDate?.formatted() ?? "Unknown")" : someWords ?? "Unknown"
                        certificateStateCell.textLabel?.numberOfLines = 0
                        certificateStateCell.selectionStyle = .none
                        UIView.animate(withDuration: 0.1) {
                            certificateStateCell.textLabel?.alpha = 1.0
                        } completion: { _ in
                            guard let self = self else { return }
                            self.isAnimatingCertState = false
                        }
                    }
                }
            }
        }
    }
}
